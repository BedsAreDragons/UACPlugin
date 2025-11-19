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

string baseUrlDatalink = "http://www.hoppie.nl/acars/system/connect.html";
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

void datalinkLogin(void * arg) {
	string raw;
	string url = baseUrlDatalink;
	url += "?logon=";
	url += logonCode;
	url += "&from=";
	url += logonCallsign;
	url += "&to=SERVER&type=PING";
	raw.assign(httpHelper->downloadStringFromURL(url));

	if (startsWith("ok", raw.c_str())) {
		HoppieConnected = true;
		ConnectionMessage = true;
	}
	else {
		FailedToConnectMessage = true;
	}
};

void sendDatalinkMessage(void * arg) {

	string raw;
	string url = baseUrlDatalink;
	url += "?logon=";
	url += logonCode;
	url += "&from=";
	url += logonCallsign;
	url += "&to=";
	url += tdest;
	url += "&type=";
	url += ttype;
	url += "&packet=";
	url += tmessage;

	size_t start_pos = 0;
	while ((start_pos = url.find(" ", start_pos)) != std::string::npos) {
		url.replace(start_pos, string(" ").length(), "%20");
		start_pos += string("%20").length();
	}

	raw.assign(httpHelper->downloadStringFromURL(url));

	if (startsWith("ok", raw.c_str())) {
		if (PendingMessages.find(DatalinkToSend.callsign) != PendingMessages.end())
			PendingMessages.erase(DatalinkToSend.callsign);
		if (std::find(AircraftMessage.begin(), AircraftMessage.end(), DatalinkToSend.callsign.c_str()) != AircraftMessage.end()) {
			AircraftMessage.erase(std::remove(AircraftMessage.begin(), AircraftMessage.end(), DatalinkToSend.callsign.c_str()), AircraftMessage.end());
		}
		AircraftMessageSent.push_back(DatalinkToSend.callsign.c_str());
	}
};

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

void sendDatalinkClearance(void * arg) {
	string raw;
	string url = baseUrlDatalink;
	url += "?logon=";
	url += logonCode;
	url += "&from=";
	url += logonCallsign;
	url += "&to=";
	url += DatalinkToSend.callsign;
	url += "&type=CPDLC&packet=/data2/";
	messageId++;
	url += std::to_string(messageId);
	url += "//R/";
	url += "CLR TO @";
	url += DatalinkToSend.destination;
	url += "@ RWY @";
	url += DatalinkToSend.rwy;
	url += "@ DEP @";
	url += DatalinkToSend.sid;
	url += "@ INIT CLB @";
	url += DatalinkToSend.climb;
	url += "@ SQUAWK @";
	url += DatalinkToSend.squawk;
	url += "@ ";
	if (DatalinkToSend.ctot != "no" && DatalinkToSend.ctot.size() > 3) {
		url += "CTOT @";
		url += DatalinkToSend.ctot;
		url += "@ ";
	}
	if (DatalinkToSend.asat != "no" && DatalinkToSend.asat.size() > 3) {
		url += "TSAT @";
		url += DatalinkToSend.asat;
		url += "@ ";
	}
	if (DatalinkToSend.freq != "no" && DatalinkToSend.freq.size() > 5) {
		url += "WHEN RDY CALL FREQ @";
		url += DatalinkToSend.freq;
		url += "@";
	}
	else {
		url += "WHEN RDY CALL @";
		url += myfrequency;
		url += "@";
	}
	url += " IF UNABLE CALL VOICE ";
	if (DatalinkToSend.message != "no" && DatalinkToSend.message.size() > 1)
		url += DatalinkToSend.message;

	size_t start_pos = 0;
	while ((start_pos = url.find(" ", start_pos)) != std::string::npos) {
		url.replace(start_pos, string(" ").length(), "%20");
		start_pos += string("%20").length();
	}

	raw.assign(httpHelper->downloadStringFromURL(url));

	if (startsWith("ok", raw.c_str())) {
		if (std::find(AircraftDemandingClearance.begin(), AircraftDemandingClearance.end(), DatalinkToSend.callsign.c_str()) != AircraftDemandingClearance.end()) {
			AircraftDemandingClearance.erase(std::remove(AircraftDemandingClearance.begin(), AircraftDemandingClearance.end(), DatalinkToSend.callsign.c_str()), AircraftDemandingClearance.end());
		}
		if (std::find(AircraftStandby.begin(), AircraftStandby.end(), DatalinkToSend.callsign.c_str()) != AircraftStandby.end()) {
			AircraftStandby.erase(std::remove(AircraftStandby.begin(), AircraftStandby.end(), DatalinkToSend.callsign.c_str()), AircraftStandby.end());
		}
		if (PendingMessages.find(DatalinkToSend.callsign) != PendingMessages.end())
			PendingMessages.erase(DatalinkToSend.callsign);
		AircraftMessageSent.push_back(DatalinkToSend.callsign.c_str());
	}
};