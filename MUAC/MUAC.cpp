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
bool MUAC::OnCompileCommand(const char * sCommandLine) {
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
			_beginthread(pollMessages, 0, NULL);
		}
		return true;
	}
	else if (strcmp(sCommandLine, ".uac log") == 0) {
		Logger::ENABLED = !Logger::ENABLED;
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
		int temp = 0;
		if (PlaySoundClr)
			temp = 1;
		SaveDataToSettings("cpdlc_sound", "Play sound on clearance request", std::to_string(temp).c_str());

		return true;
	}
	return false;
}


void pollMessages(void * arg) {
	string raw = "";
	string url = baseUrlDatalink;
	url += "?logon=";
	url += logonCode;
	url += "&from=";
	url += logonCallsign;
	url += "&to=SERVER&type=POLL";
	raw.assign(httpHelper->downloadStringFromURL(url));

	if (!startsWith("ok", raw.c_str()) || raw.size() <= 3)
		return;

	raw = raw + " ";
	raw = raw.substr(3, raw.size() - 3);

	string delimiter = "}} ";
	size_t pos = 0;
	std::string token;
	while ((pos = raw.find(delimiter)) != std::string::npos) {
		token = raw.substr(1, pos);

		string parsed;
		stringstream input_stringstream(token);
		struct AcarsMessage message;
		int i = 1;
		while (getline(input_stringstream, parsed, ' '))
		{
			if (i == 1)
				message.from = parsed;
			if (i == 2)
				message.type = parsed;
			if (i > 2)
			{
				message.message.append(" ");
				message.message.append(parsed);
			}

			i++;
		}
		if (message.type.find("telex") != std::string::npos || message.type.find("cpdlc") != std::string::npos) {
			if (message.message.find("REQ") != std::string::npos || message.message.find("CLR") != std::string::npos || message.message.find("PDC") != std::string::npos || message.message.find("PREDEP") != std::string::npos || message.message.find("REQUEST") != std::string::npos) {
				if (message.message.find("LOGON") != std::string::npos) {
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
			else if (message.message.find("WILCO") != std::string::npos || message.message.find("ROGER") != std::string::npos || message.message.find("RGR") != std::string::npos) {
				if (std::find(AircraftMessageSent.begin(), AircraftMessageSent.end(), message.from) != AircraftMessageSent.end()) {
					AircraftWilco.push_back(message.from);
				}
			}
			else if (message.message.length() != 0 ){
				AircraftMessage.push_back(message.from);
			}
			PendingMessages[message.from] = message;
		}

		raw.erase(0, pos + delimiter.length());
	}


};

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
