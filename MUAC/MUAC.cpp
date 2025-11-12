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

// ------------------------
// MUAC class
// ------------------------

MUAC* MUAC::Instance = nullptr;  // <-- static pointer definition

MUAC::MUAC()
    : CPlugIn(COMPATIBILITY_CODE, PLUGIN_NAME.c_str(),
              PLUGIN_VERSION.c_str(), PLUGIN_AUTHOR.c_str(), PLUGIN_COPY.c_str())
{
    MUAC::Instance = this;  // <-- assign inside constructor

    srand(static_cast<unsigned int>(time(nullptr)));
    RegisterPlugin();

    DisplayUserMessage("MUAC", "MUAC PlugIn",
        ("Version " + PLUGIN_VERSION + " loaded").c_str(),
        false, false, false, false, false);

    if (!httpHelper)
        httpHelper = new HttpHelper();

    // Initialize random message ID
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
    if (strncmp(".hoppie connect", sCommandLine, 15) == 0)
    {
        if (!HoppieConnected)
        {
            // <-- create the dialog correctly
            CPDLCSettingsDialog dlg(nullptr);  
            dlg.m_Logon = logonCallsign.c_str();
            dlg.m_Password = logonCode.c_str();
            dlg.m_Sound = PlaySoundClr ? 1 : 0;

            if (dlg.DoModal() == IDOK)
            {
                logonCallsign = dlg.m_Logon;
                logonCode = dlg.m_Password;
                PlaySoundClr = dlg.m_Sound != 0;

                // Save settings
                SaveDataToSettings("hoppie_logon", "Hoppie ACARS Callsign", logonCallsign.c_str());
                SaveDataToSettings("hoppie_code", "Hoppie ACARS Logon Code", logonCode.c_str());
                SaveDataToSettings("hoppie_sound", "Play sound on clearance request", to_string(PlaySoundClr).c_str());

                // Start login thread
                thread loginThread(datalinkLogin);
                loginThread.detach();
            }
        }
        else
        {
            HoppieConnected = false;
            DisplayUserMessage("Hoppie ACARS", "Server", "Logged off!", true, true, false, true, false);
        }

        return true;
    }

    return false;
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
        // Periodically poll Hoppie server
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

    string url = baseUrlDatalink + "?logon=" + logonCode + "&from=" + logonCallsign + "&to=" + tdest + "&type=" + ttype + "&packet=" + tmessage;

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

// ------------------------
// Poll messages from Hoppie
// ------------------------
void pollHoppieMessages()
{
    if (!httpHelper) return;

    string url = baseUrlDatalink + "?logon=" + logonCode + "&from=" + logonCallsign + "&to=SERVER&type=POLL";
    string raw = httpHelper->downloadStringFromURL(url);

    if (raw.find("ok") != 0 || raw.size() <= 3) return;

    // TODO: parse messages from raw string
}
