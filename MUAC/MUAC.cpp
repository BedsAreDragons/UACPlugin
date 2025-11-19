#include "stdafx.h"
#include "MUAC.h"
#include "CPDLCSettingsDialog.h"  
#include "HttpHelper.h"
#include "Logger.h"
#include "CallsignLookup.h"
#include "RadarScreen.h"

#include <ctime>
#include <cstdlib>
#include <thread>
#include <future>
#include <sstream>
#include <algorithm>

using namespace std;

// --- Globals ---
future<string> fRDFString;

HttpHelper* httpHelper = nullptr;
bool HoppieConnected = false;
bool ConnectionMessage = false;
bool FailedToConnectMessage = false;

bool PlaySoundClr = true;
string logonCallsign = "EGKK";
string logonCode = "";

string baseUrlDatalink = "https://hoppie.acars.server/api";
string tdest, ttype, tmessage;
static int messageId = 0;

// Forward declarations
void datalinkLogin();
void sendHoppieMessage();
void pollHoppieMessages();
void pollMessages(void* arg);
void sendDatalinkMessage(void* arg);

// ------------------------
// MUAC class
// ------------------------

MUAC* MUAC::Instance = nullptr;  // <-- static pointer definition

MUAC::MUAC()
    : CPlugIn(COMPATIBILITY_CODE, PLUGIN_NAME.c_str(),
              PLUGIN_VERSION.c_str(), PLUGIN_AUTHOR.c_str(), PLUGIN_COPY.c_str())
{
    MUAC::Instance = this;

    srand(static_cast<unsigned int>(time(nullptr)));
    RegisterPlugin();

    DisplayUserMessage("MUAC", "MUAC PlugIn",
        ("Version " + PLUGIN_VERSION + " loaded").c_str(),
        false, false, false, false, false);

    if (!httpHelper)
        httpHelper = new HttpHelper();

    messageId = rand() % 10000 + 1000;

    Logger::info("MUAC Datalink initialized.");
}

MUAC::~MUAC()
{
    if (httpHelper)
    {
        delete httpHelper;
        httpHelper = nullptr;
    }
}

// ------------------------
// Plugin commands
// ------------------------
bool MUAC::OnCompileCommand(const char* sCommandLine)
{
    if (startsWith(".uac connect", sCommandLine))
    {
        if (ControllerMyself().IsController()) {
            if (!HoppieConnected) {
                _beginthread(datalinkLogin, 0, NULL);
            }
            else {
                HoppieConnected = false;
                DisplayUserMessage("CPDLC", "Server", "Logged off!", true, true, false, true, false);
            }
        }
        else {
            DisplayUserMessage("CPDLC", "Error", "You are not logged in as a controller!", true, true, false, true, false);
        }
        return true;
    }
    else if (startsWith(".uac poll", sCommandLine))
    {
        if (HoppieConnected) {
            _beginthread(pollHoppieMessages, 0, NULL);
        }
        else {
            DisplayUserMessage("CPDLC", "Server", "Not connected to Hoppie!", true, true, false, true, false);
        }
        return true;
    }
    else if (strcmp(sCommandLine, ".uac log") == 0) {
        Logger::ENABLED = !Logger::ENABLED;
        DisplayUserMessage("CPDLC", "Logging", Logger::ENABLED ? "Enabled" : "Disabled", true, true, false, true, false);
        return true;
    }
    else if (startsWith(".uac", sCommandLine))
    {
        CPDLCSettingsDialog dia;
        dia.m_Logon = logonCallsign.c_str();
        dia.m_Password = logonCode.c_str();
        dia.m_Sound = int(PlaySoundClr);

        if (dia.DoModal() != IDOK)
            return true;

        logonCallsign = dia.m_Logon;
        logonCode = dia.m_Password;
        PlaySoundClr = bool(!!dia.m_Sound);

        SaveDataToSettings("cpdlc_logon", "The CPDLC logon callsign", logonCallsign.c_str());
        SaveDataToSettings("cpdlc_password", "The CPDLC logon password", logonCode.c_str());
        SaveDataToSettings("cpdlc_sound", "Play sound on clearance request", PlaySoundClr ? "1" : "0");

        DisplayUserMessage("CPDLC", "Settings", "Saved!", true, true, false, true, false);
        return true;
    }
    return false;
}

// ------------------------
// Poll Hoppie messages
// ------------------------
void pollHoppieMessages()
{
    if (!httpHelper) return;

    string url = baseUrlDatalink + "?logon=" + logonCode + "&from=" + logonCallsign + "&to=SERVER&type=POLL";
    string raw = httpHelper->downloadStringFromURL(url);

    if (raw.find("ok") != 0 || raw.size() <= 3) return;

    raw = raw.substr(2); // remove "ok"

    string delimiter = "}} ";
    size_t pos = 0;
    string token;
    while ((pos = raw.find(delimiter)) != string::npos) {
        token = raw.substr(1, pos);

        stringstream input_stringstream(token);
        struct AcarsMessage message;
        string parsed;
        int i = 1;
        while (getline(input_stringstream, parsed, ' ')) {
            if (i == 1) message.from = parsed;
            else if (i == 2) message.type = parsed;
            else message.message += " " + parsed;
            i++;
        }

        if (message.type.find("telex") != string::npos || message.type.find("cpdlc") != string::npos) {
            if (message.message.find("REQ") != string::npos || message.message.find("CLR") != string::npos ||
                message.message.find("PDC") != string::npos || message.message.find("PREDEP") != string::npos ||
                message.message.find("REQUEST") != string::npos) {

                if (message.message.find("LOGON") != string::npos) {
                    tmessage = "UNABLE";
                    ttype = "CPDLC";
                    tdest = DatalinkToSend.callsign;
                    _beginthread(sendDatalinkMessage, 0, NULL);
                } else {
                    if (PlaySoundClr) {
                        AFX_MANAGE_STATE(AfxGetStaticModuleState());
                        PlaySound(MAKEINTRESOURCE(IDR_WAVE1), AfxGetInstanceHandle(), SND_RESOURCE | SND_ASYNC);
                    }
                    AircraftDemandingClearance.push_back(message.from);
                }
            }
            else if (message.message.find("WILCO") != string::npos ||
                     message.message.find("ROGER") != string::npos ||
                     message.message.find("RGR") != string::npos) {
                if (std::find(AircraftMessageSent.begin(), AircraftMessageSent.end(), message.from) != AircraftMessageSent.end()) {
                    AircraftWilco.push_back(message.from);
                }
            }
            else if (!message.message.empty()) {
                AircraftMessage.push_back(message.from);
            }
            PendingMessages[message.from] = message;
        }

        raw.erase(0, pos + delimiter.length());
    }
}

// ------------------------
// Radar screen creation
// ------------------------
CRadarScreen* MUAC::OnRadarScreenCreated(const char* sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated)
{
    if (strcmp(sDisplayName, MUAC_RADAR_SCREEN_VIEW) == 0)
        return new RadarScreen();

    return nullptr;
}

// ------------------------
// Timer
// ------------------------
void MUAC::OnTimer(int Counter)
{
    if (Counter % 5 == 0)
    {
        thread pollThread(pollHoppieMessages);
        pollThread.detach();
    }
}

// ------------------------
// Register plugin display types
// ------------------------
void MUAC::RegisterPlugin()
{
    RegisterDisplayType(MUAC_RADAR_SCREEN_VIEW, false, true, true, true);
}

// ------------------------
// Hoppie datalink login
// ------------------------
void datalinkLogin()
{
    if (!httpHelper) return;

    string url = baseUrlDatalink + "?logon=" + logonCode + "&from=" + logonCallsign + "&to=SERVER&type=PING";
    string raw = httpHelper->downloadStringFromURL(url);

    if (raw.find("ok") == 0)
    {
        HoppieConnected = true;
        ConnectionMessage = true;

        if (MUAC::Instance)
            MUAC::Instance->DisplayUserMessage("Hoppie ACARS", "Server", "Connected!", true, true, false, true, false);
    }
    else
    {
        FailedToConnectMessage = true;
        if (MUAC::Instance)
            MUAC::Instance->DisplayUserMessage("Hoppie ACARS", "Server", "Failed to connect!", true, true, false, true, false);
    }
}

// ------------------------
// Send Hoppie message
// ------------------------
void sendHoppieMessage()
{
    if (!httpHelper) return;

    string url = baseUrlDatalink + "?logon=" + logonCode + "&from=" + logonCallsign +
                 "&to=" + tdest + "&type=" + ttype + "&packet=" + tmessage;

    size_t pos = 0;
    while ((pos = url.find(" ", pos)) != string::npos)
    {
        url.replace(pos, 1, "%20");
        pos += 3;
    }

    string raw = httpHelper->downloadStringFromURL(url);

    if (raw.find("ok") == 0)
    {
        // Successfully sent
    }
}

