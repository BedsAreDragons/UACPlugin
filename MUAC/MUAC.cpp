#include "stdafx.h"
#include "MUAC.h"
#include "HttpHelper.h"  // <-- needed
#include "Logger.h"      // <-- needed

#include <string>
#include <vector>
#include <map>
#include <future>
#include <cstdlib>
#include <ctime>
#include <sstream>

using namespace std;

// Globals
future<string> fRDFString;

// Define static members
bool Logger::ENABLED = false;
std::string Logger::DLL_PATH = "";


bool HoppieConnected = false;
bool ConnectionMessage = false;
bool FailedToConnectMessage = false;

string logonCode = "";
string logonCallsign = "EGKK";

HttpHelper* httpHelper = nullptr;
static int messageId = 0;

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
   Logger::info("MUAC Datalink initialized.");

}

MUAC::~MUAC() {}

bool MUAC::OnCompileCommand(const char* sCommandLine) {
    if (startsWith(".hoppie connect", sCommandLine)) {
        if (!HoppieConnected) {
            CHoppieLogonDialog dlg;
            dlg.m_LogonCallsign = logonCallsign.c_str();
            dlg.m_LogonCode = logonCode.c_str();
            dlg.m_PlaySound = PlaySoundClr;

            if (dlg.DoModal() == IDOK) {
                logonCallsign = dlg.m_LogonCallsign;
                logonCode = dlg.m_LogonCode;
                PlaySoundClr = dlg.m_PlaySound;

                // save to settings
                SaveDataToSettings("hoppie_logon", "Hoppie ACARS Callsign", logonCallsign.c_str());
                SaveDataToSettings("hoppie_code", "Hoppie ACARS Logon Code", logonCode.c_str());

                // perform login
                _beginthread(datalinkLogin, 0, nullptr);
            }
        } else {
            HoppieConnected = false;
            DisplayUserMessage("Hoppie ACARS", "Server", "Logged off!", true, true, false, true, false);
        }
        return true;
    }

    // other commands...
    return false;
}


void sendHoppieMessage(void* arg) {
    string raw;
    string url = baseUrlDatalink;
    url += "?logon=" + logonCode;
    url += "&from=" + logonCallsign;
    url += "&to=" + tdest;
    url += "&type=" + ttype;
    url += "&packet=" + tmessage;

    // Encode spaces
    size_t pos = 0;
    while ((pos = url.find(" ", pos)) != string::npos) {
        url.replace(pos, 1, "%20");
        pos += 3;
    }

    raw = httpHelper->downloadStringFromURL(url);

    if (startsWith("ok", raw.c_str())) {
        // handle successful send
    }
}

void pollHoppieMessages(void* arg) {
    string url = baseUrlDatalink + "?logon=" + logonCode + "&from=" + logonCallsign + "&to=SERVER&type=POLL";
    string raw = httpHelper->downloadStringFromURL(url);

    if (!startsWith("ok", raw.c_str()) || raw.size() <= 3) return;

    // parse messages like in your example
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
