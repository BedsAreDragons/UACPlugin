#include "stdafx.h"
#include "MUAC.h"

future<string> fRDFString;

bool HoppieConnected = false;
bool ConnectionMessage = false;
bool FailedToConnectMessage = false;

string logonCode = "";
string logonCallsign = "EGKK";

HttpHelper * httpHelper = NULL;


MUAC::MUAC():CPlugIn(COMPATIBILITY_CODE, PLUGIN_NAME.c_str(), PLUGIN_VERSION.c_str(), PLUGIN_AUTHOR.c_str(), PLUGIN_COPY.c_str()) {

	srand((unsigned int)time(nullptr));
	this->RegisterPlugin();

	DisplayUserMessage("Message", "MUAC PlugIn", string("Version " + PLUGIN_VERSION + " loaded").c_str(), false, false, false, false, false);

	char DllPathFile[_MAX_PATH];
	string DllPath;
	GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
	DllPath = DllPathFile;
	DllPath.resize(DllPath.size() - strlen("MUAC.dll"));

	string FilePath = DllPath + "\\ICAO_Airlines.txt";
	if (file_exist(FilePath)) {
		CCallsignLookup::Lookup = new CCallsignLookup(FilePath);
		CCallsignLookup::Available = true;
	}
	else {
		CCallsignLookup::Available = false;
		DisplayUserMessage("Message", "MUAC PlugIn", string("Warning: Could not load callsigns, they will be unavailable").c_str(), 
			true, true, false, false, true);
	}

}

MUAC::~MUAC() {}

// ============================================================================
// Data Structures
// ============================================================================
struct DatalinkPacket {
	string callsign;
	string destination;
	string sid;
	string rwy;
	string freq;
	string ctot;
	string asat;
	string squawk;
	string message;
	string climb;
};

struct AcarsMessage {
	string from;
	string type;
	string message;
};

// ============================================================================
// Globals
// ============================================================================

static DatalinkPacket DatalinkToSend;

static string logonCallsign;
static string logonCode;
static string myFrequency = "131.225";

static bool MUACConnected = false;
static bool PlaySoundClr = true;

static vector<string> AircraftDemandingClearance;
static vector<string> AircraftMessageSent;
static vector<string> AircraftMessage;
static vector<string> AircraftWilco;
static vector<string> AircraftStandby;
static map<string, AcarsMessage> PendingMessages;

static int messageId = 0;

// ============================================================================
// Helpers
// ============================================================================
static inline string URLEncodeSpaces(string s) {
	size_t pos = 0;
	while ((pos = s.find(' ', pos)) != string::npos) {
		s.replace(pos, 1, "%20");
		pos += 3;
	}
	return s;
}

static inline bool startsWith(const string& s, const string& prefix) {
	return s.rfind(prefix, 0) == 0;
}

// ============================================================================
// MUAC Class Implementation
// ============================================================================
MUAC::MUAC() :
	CPlugIn(COMPATIBILITY_CODE, PLUGIN_NAME.c_str(), PLUGIN_VERSION.c_str(),
		PLUGIN_AUTHOR.c_str(), PLUGIN_COPY.c_str())
{
	srand((unsigned int)time(nullptr));
	this->RegisterPlugin();

	DisplayUserMessage("MUAC", "MUAC PlugIn",
		string("Version " + PLUGIN_VERSION + " loaded").c_str(),
		false, false, false, false, false);

	char DllPathFile[_MAX_PATH];
	string DllPath;
	GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
	DllPath = DllPathFile;
	DllPath.resize(DllPath.size() - strlen("MUAC.dll"));

	if (!httpHelper)
		httpHelper = new HttpHelper();

	messageId = rand() % 10000 + 1000;
	Logger::log("MUAC Datalink initialized.");
}

// ============================================================================
// Datalink Functions
// ============================================================================
void MUAC::DatalinkLogin() {
	string url = baseUrlDatalink + "/login?logon=" + logonCode + "&from=" + logonCallsign;
	string raw = httpHelper->downloadStringFromURL(url);

	if (startsWith(raw, "ok")) {
		MUACConnected = true;
		DisplayUserMessage("MUAC", "Datalink", "Connected to MUAC Datalink system.", true, false, false, true, false);
		Logger::log("MUAC datalink connected.");
	}
	else {
		MUACConnected = false;
		DisplayUserMessage("MUAC", "Datalink", "Connection failed.", true, false, false, true, false);
		Logger::log("MUAC datalink connection failed.");
	}
}

void MUAC::SendDatalinkMessage(const string& dest, const string& type, const string& message) {
	if (!MUACConnected)
		return;

	string url = baseUrlDatalink + "/send?logon=" + logonCode +
		"&from=" + logonCallsign +
		"&to=" + dest +
		"&type=" + type +
		"&packet=" + message;

	url = URLEncodeSpaces(url);
	string raw = httpHelper->downloadStringFromURL(url);

	if (startsWith(raw, "ok")) {
		PendingMessages.erase(dest);
		AircraftMessageSent.push_back(dest);
		Logger::log("Message sent successfully to " + dest);
	}
}

void MUAC::PollMessages() {
	if (!MUACConnected)
		return;

	string url = baseUrlDatalink + "/poll?logon=" + logonCode + "&from=" + logonCallsign;
	string raw = httpHelper->downloadStringFromURL(url);

	if (!startsWith(raw, "ok") || raw.size() <= 3)
		return;

	raw = raw.substr(3);
	string delimiter = "}} ";
	size_t pos = 0;

	while ((pos = raw.find(delimiter)) != string::npos) {
		string token = raw.substr(1, pos - 1);
		raw.erase(0, pos + delimiter.length());

		stringstream ss(token);
		AcarsMessage msg;
		string part;
		int field = 0;

		while (getline(ss, part, ' ')) {
			if (field == 0) msg.from = part;
			else if (field == 1) msg.type = part;
			else msg.message += (msg.message.empty() ? "" : " ") + part;
			field++;
		}

		if (msg.type.find("cpdlc") != string::npos) {
			if (msg.message.find("REQ") != string::npos || msg.message.find("CLR") != string::npos) {
				AircraftDemandingClearance.push_back(msg.from);
				DisplayUserMessage("MUAC", "Datalink", ("Clearance request from " + msg.from).c_str(),
					true, false, false, true, false);
			}
			else if (msg.message.find("WILCO") != string::npos) {
				AircraftWilco.push_back(msg.from);
			}
			else {
				AircraftMessage.push_back(msg.from);
			}
			PendingMessages[msg.from] = msg;
		}
	}
}

void MUAC::SendClearance() {
	if (!MUACConnected)
		return;

	string url = baseUrlDatalink + "/send?logon=" + logonCode +
		"&from=" + logonCallsign +
		"&to=" + DatalinkToSend.callsign +
		"&type=CPDLC&packet=/data2/" + to_string(++messageId) +
		"//R/CLR TO @" + DatalinkToSend.destination +
		"@ RWY @" + DatalinkToSend.rwy +
		"@ DEP @" + DatalinkToSend.sid +
		"@ INIT CLB @" + DatalinkToSend.climb +
		"@ SQUAWK @" + DatalinkToSend.squawk + "@ ";

	if (DatalinkToSend.ctot.size() > 3)
		url += "CTOT @" + DatalinkToSend.ctot + "@ ";
	if (DatalinkToSend.asat.size() > 3)
		url += "TSAT @" + DatalinkToSend.asat + "@ ";
	url += "WHEN RDY CALL @" + myFrequency + "@ IF UNABLE CALL VOICE";

	url = URLEncodeSpaces(url);
	string raw = httpHelper->downloadStringFromURL(url);

	if (startsWith(raw, "ok")) {
		AircraftMessageSent.push_back(DatalinkToSend.callsign);
		DisplayUserMessage("MUAC", "Datalink", ("Sent clearance to " + DatalinkToSend.callsign).c_str(),
			true, false, false, true, false);
		Logger::log("Clearance sent to " + DatalinkToSend.callsign);
	}
}

CRadarScreen * MUAC::OnRadarScreenCreated(const char * sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated)
{
	if (!strcmp(sDisplayName, MUAC_RADAR_SCREEN_VIEW))
		return new RadarScreen();

	return nullptr;
}

void MUAC::OnTimer(int Counter)
{
	if (Counter % 5 == 0) {

	}
}

void MUAC::RegisterPlugin() {
	RegisterDisplayType(MUAC_RADAR_SCREEN_VIEW, false, true, true, true);
}
