#include "stdafx.h"
#include "EuroScopePlugIn.h"         // DisplayUserMessage, CPlugIn
#include "MUAC.h"
#include "HttpHelper.h"
#include "Logger.h"
#include "CPDLCSettingsDialog.hpp"

#include <string>
#include <future>
#include <ctime>
#include <cstdlib>
#include <sstream>

using namespace std;

// Globals
future<string> fRDFString;

void datalinkLogin(void* arg);  // forward declaration

// Hoppie CPDLC globals
string baseUrlDatalink = "https://hoppie.acars.server/api";
string tdest;
string ttype;
string tmessage;
bool PlaySoundClr = true;

bool HoppieConnected = false;
bool ConnectionMessage = false;
bool FailedToConnectMessage = false;

string logonCode = "";
string logonCallsign = "EGKK";

HttpHelper* httpHelper = nullptr;
static int messageId = 0;

// Logger static members
bool Logger::ENABLED = false;
string Logger::DLL_PATH = "";

// ----------------- Functions -----------------
void datalinkLogin(void* arg) {
    if (!httpHelper) return;

    string url = baseUrlDatalink + "?logon=" + logonCode + "&from=" + logonCallsign + "&to=SERVER&type=PING";
    string raw = httpHelper->downloadStringFromURL(url);

    if (raw.find("ok") == 0) {
        HoppieConnected = true;
        ConnectionMessage = true;
        DisplayUserMessage("Hoppie ACARS", "Server", "Connected!", true, true, false, true, false);
    } else {
        FailedToConnectMessage = true;
        DisplayUserMessage("Hoppie ACARS", "Server", "Failed to connect!", true, true, false, true, false);
    }
}

bool MUAC::OnCompileCommand(const char* sCommandLine) {
    if (startsWith(".hoppie connect", sCommandLine)) {
        if (!HoppieConnected) {
            CCPDLCSettingsDialog dlg;
            dlg.m_Logon = logonCallsign.c_str();
            dlg.m_Password = logonCode.c_str();
            dlg.m_Sound = PlaySoundClr ? 1 : 0;

            if (dlg.DoModal() == IDOK) {
                logonCallsign = dlg.m_Logon;
                logonCode = dlg.m_Password;
                PlaySoundClr = dlg.m_Sound != 0;

                // save settings
                SaveDataToSettings("hoppie_logon", "Hoppie ACARS Callsign", logonCallsign.c_str());
                SaveDataToSettings("hoppie_code", "Hoppie ACARS Logon Code", logonCode.c_str());
                SaveDataToSettings("hoppie_sound", "Play sound on clearance request", to_string(PlaySoundClr).c_str());

                // perform login
                _beginthread(datalinkLogin, 0, nullptr);
            }
        } else {
            HoppieConnected = false;
            DisplayUserMessage("Hoppie ACARS", "Server", "Logged off!", true, true, false, true, false);
        }
        return true;
    }

    return false;
}

// Other MUAC functions remain the same, e.g., OnRadarScreenCreated, OnTimer, RegisterPlugin
