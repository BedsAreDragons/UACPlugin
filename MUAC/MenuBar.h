#pragma once
#include "stdafx.h"
#include "Constants.h"
#include "Colours.h"
#include <vector>
#include <map>
#include <string>
#include "EuroScopePlugIn.h"

using namespace std;
using namespace EuroScopePlugIn;

class MenuBar
{
private:
    const static int ButtonPaddingSides = 5;
    const static int ButtonPaddingTop = 2;

public:
    static map<int, string> MakeButtonData() {
        map<int, string> data;
        data[BUTTON_HIDEMENU] = "X";
        data[BUTTON_DECREASE_RANGE] = "-";
        data[BUTTON_RANGE] = "120 =";
        data[BUTTON_INCREASE_RANGE] = "+";
        data[BUTTON_FILTER_ON] = "FILTER ON";
        data[BUTTON_FILTER_HARD_LOW] = "0 =";
        data[BUTTON_FILTER_SOFT_LOW] = "200 =";
        data[BUTTON_FILTER_SOFT_HIGH] = "999 =";
        data[BUTTON_FILTER_HARD_HIGH] = "999 =";
        data[BUTTON_MTCD] = "MTCD";
        data[BUTTON_FIM] = "VAR FIM";
        data[BUTTON_QDM] = "QDM";
        data[BUTTON_TOPDOWN] = "T";
        data[BUTTON_MODE_A] = "A";
        data[BUTTON_LABEL_V] = "V";
        data[BUTTON_PRIMARY_TARGETS_ON] = "PR";
        data[BUTTON_VFR_ON] = "VFR";
        data[BUTTON_VELOTH] = "VEL OTH";
        data[BUTTON_VEL1] = "1";
        data[BUTTON_VEL2] = "2";
        data[BUTTON_VEL4] = "4";
        data[BUTTON_VEL8] = "8";
        data[BUTTON_DOTS] = "DOTS";
        data[BUTTON_FIN] = "FIN";
        return data;
    }

    static void DrawMenuBar(CDC* dc, CRadarScreen* radarScreen, POINT TopRight, POINT MousePt, map<int, string> ButtonData, map<int, bool> PressedData) {
        int TopOffset = TopRight.y;
        int RightEdge = TopRight.x;

        for (auto kv : ButtonData) {
            CRect sizeRect = CalculateButtonRect(dc, kv.second);
            int ButtonWidth = sizeRect.Width();
            int ButtonHeight = sizeRect.Height();

            CRect Button(RightEdge - ButtonWidth, TopOffset, RightEdge, TopOffset + ButtonHeight);

            // Fill button background
            if (PressedData[kv.first])
                dc->FillSolidRect(Button, Colours::LightBlueMenu.ToCOLORREF());
            else
                dc->FillSolidRect(Button, Colours::DarkBlueMenu.ToCOLORREF());

            // Draw 3D border
            dc->Draw3dRect(Button, Colours::MenuButtonTop.ToCOLORREF(), Colours::MenuButtonBottom.ToCOLORREF());

            // Highlight on hover
            if (IsInRect(MousePt, Button)) {
                CPen YellowPen(PS_SOLID, 1, Colours::YellowHighlight.ToCOLORREF());
                dc->SelectStockObject(NULL_BRUSH);
                dc->SelectObject(&YellowPen);
                dc->Rectangle(Button);
            }

            // Draw text
            dc->SetTextColor(PressedData[kv.first] ? Colours::DarkBlueMenu.ToCOLORREF() : Colours::GreyTextMenu.ToCOLORREF());
            dc->TextOutA(Button.left + ButtonPaddingSides, Button.top + ButtonPaddingTop, kv.second.c_str());

            radarScreen->AddScreenObject(kv.first, "", Button, false, "");

            // Move down for next button
            TopOffset += ButtonHeight;

            if (PressedData[BUTTON_HIDEMENU])
                break;
        }
    }

    static CRect CalculateButtonRect(CDC* dc, string Text) {
        CSize TextSize = dc->GetTextExtent(Text.c_str());
        int Width = TextSize.cx + ButtonPaddingSides * 2;
        int Height = TextSize.cy + ButtonPaddingTop * 2;
        return CRect(0, 0, Width, Height);
    }

    static map<int, bool> ResetAllVelButtons(map<int, bool> Data) {
        Data[BUTTON_VEL1] = false;
        Data[BUTTON_VEL2] = false;
        Data[BUTTON_VEL4] = false;
        Data[BUTTON_VEL8] = false;
        return Data;
    }

    static int GetVelValueButtonPressed(map<int, bool> Data) {
        if (Data[BUTTON_VEL1]) return 60;
        if (Data[BUTTON_VEL2]) return 60 * 2;
        if (Data[BUTTON_VEL4]) return 60 * 4;
        if (Data[BUTTON_VEL8]) return 60 * 8;
        return 0;
    }

    static map<int, bool> LoadVelValueToButtons(int value, map<int, bool> Data) {
        Data = ResetAllVelButtons(Data);
        if (value == 60) Data[BUTTON_VEL1] = true;
        if (value == 60 * 2) Data[BUTTON_VEL2] = true;
        if (value == 60 * 4) Data[BUTTON_VEL4] = true;
        if (value == 60 * 8) Data[BUTTON_VEL8] = true;
        return Data;
    }
};