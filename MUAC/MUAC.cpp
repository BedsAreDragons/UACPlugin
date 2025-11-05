#include "stdafx.h"
#include "MUAC.h"

future<string> fRDFString;

bool HoppieConnected = false;
bool ConnectionMessage = false;
bool FailedToConnectMessage = false;

string logonCode = "";
string logonCallsign = "EGKK";

HttpHelper* httpHelper = nullptr;

MUAC::MUAC() :
    CPlugIn(COMPATIBILITY_CODE, PLUGIN_NAME.c_str(),
        PLUGIN_VERSION.c_str(), PLUGIN_AUTHOR.c_str(), PLUGIN_COPY.c_str())
{
    srand((unsigned int)time(nullptr));
    this->RegisterPlugin();

    DisplayUserMessage("MUAC", "MUAC PlugIn",
        string("Version " + PLUGIN_VERSION + " loaded").c_str(),
        false, false, false, false, false);

    // ---- Find DLL path ----
    char DllPathFile[_MAX_PATH];
    string DllPath;
    GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
    DllPath = DllPathFile;
    DllPath.resize(DllPath.size() - strlen("MUAC.dll"));

    // ---- Load Callsign Lookup ----
    string FilePath = DllPath + "\\ICAO_Airlines.txt";
    if (file_exist(FilePath)) {
        CCallsignLookup::Lookup = new CCallsignLookup(FilePath);
        CCallsignLookup::Available = true;
    }
    else {
        CCallsignLookup::Available = false;
        DisplayUserMessage("MUAC", "MUAC PlugIn",
            "Warning: Could not load callsigns, they will be unavailable",
            true, true, false, false, true);
    }

    // ---- Init HTTP Helper ----
    if (!httpHelper)
        httpHelper = new HttpHelper();

    // ---- Init Random ID and Logging ----
    messageId = rand() % 10000 + 1000;
    Logger::log("MUAC Datalink initialized.");
}

MUAC::~MUAC() {}

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
