#pragma once
#include "stdafx.h"
#include "Colours.h"
#include "EuroScopePlugIn.h"

using namespace std;
using namespace Gdiplus;
using namespace EuroScopePlugIn;

class AcSymbols
{
public:
	static CRect DrawSquareAndTrail(CDC* dc, TagConfiguration::TagStates State, CRadarScreen* radar, CRadarTarget radarTarget, bool drawTrail, bool isSoft, bool isStca, bool blink, bool isDetailed) {
		int save = dc->SaveDC();
		
		COLORREF SymbolColor = Colours::AircraftDarkGrey.ToCOLORREF();
		COLORREF TrailColor = Colours::AircraftDarkGrey.ToCOLORREF();

		if (!isSoft) {
			TrailColor = Colours::AircraftLightGrey.ToCOLORREF();
			SymbolColor = Colours::AircraftLightGrey.ToCOLORREF();
		}

		if (State == TagConfiguration::TagStates::Assumed) {
			TrailColor = Colours::AircraftGreen.ToCOLORREF();
			SymbolColor = Colours::AircraftGreen.ToCOLORREF();
		}

		if (State == TagConfiguration::TagStates::Next)
			SymbolColor = Colours::AircraftGreen.ToCOLORREF();

		if (State == TagConfiguration::TagStates::TransferredToMe) {
			SymbolColor = Colours::AircraftGreen.ToCOLORREF();

			if (blink)
				SymbolColor = Colours::AircraftLightGrey.ToCOLORREF();
		}

		if (State == TagConfiguration::TagStates::Redundant)
			SymbolColor = Colours::AircraftBlue.ToCOLORREF();

		if (isStca) {
			if (blink)
				SymbolColor = Colours::YellowHighlight.ToCOLORREF();
			else
				SymbolColor = Colours::RedWarning.ToCOLORREF();
		}

		CPen TrailPen(PS_SOLID, 1, TrailColor);
		CPen SymbolPen(PS_SOLID, 1, SymbolColor);

		dc->SelectObject(&TrailPen);
		CBrush* oldBrush = (CBrush*)dc->SelectStockObject(NULL_BRUSH);

		// History Trail
		CRadarTargetPositionData TrailPosition = radarTarget.GetPreviousPosition(radarTarget.GetPreviousPosition(radarTarget.GetPosition()));
		for (int i = 0; i < 5; i++) {
			if (!drawTrail)
				break;

			POINT historyTrailPoint = radar->ConvertCoordFromPositionToPixel(TrailPosition.GetPosition());

			CRect Area(
				historyTrailPoint.x - DRAWING_AC_SQUARE_TRAIL_SIZE,
				historyTrailPoint.y - DRAWING_AC_SQUARE_TRAIL_SIZE,
				historyTrailPoint.x + DRAWING_AC_SQUARE_TRAIL_SIZE,
				historyTrailPoint.y + DRAWING_AC_SQUARE_TRAIL_SIZE
			);
			Area.NormalizeRect();

			// Draw circle instead of square
			dc->Ellipse(Area);

			// Skip one position for the other
			TrailPosition = radarTarget.GetPreviousPosition(radarTarget.GetPreviousPosition(TrailPosition));
		}
			
		dc->SelectObject(&SymbolPen);

		// Symbol itself
		POINT radarTargetPoint = radar->ConvertCoordFromPositionToPixel(radarTarget.GetPosition().GetPosition());

		int Size = DRAWING_AC_SQUARE_SYMBOL_SIZE;

		// Bigger target if ident
		if (isDetailed || radarTarget.GetPosition().GetTransponderI())
			Size += static_cast<int>(Size * 0.25);

		CRect Area(
			radarTargetPoint.x - Size,
			radarTargetPoint.y - Size,
			radarTargetPoint.x + Size,
			radarTargetPoint.y + Size
		);
		Area.NormalizeRect();

		// if STCA or Ident, filled in target
		if (isStca || radarTarget.GetPosition().GetTransponderI()) {
			CBrush brush(SymbolColor);
			CBrush* pOldBrush = dc->SelectObject(&brush);

			// Draw filled circle
			dc->Ellipse(Area);

			dc->SelectObject(pOldBrush);
		}
		else {
			// Draw outline circle
			dc->Ellipse(Area);
		}
		
		// Pixel in center (optional)
		// dc->SetPixel(Area.CenterPoint(), SymbolColor);

		dc->RestoreDC(save);

		return Area;
	}

	static CRect DrawPrimaryTrailAndDiamong(CDC* dc, CRadarScreen* radar, CRadarTarget radarTarget, bool drawTrail) {
		int save = dc->SaveDC();

		COLORREF TrailColor = Colours::AircraftLightGrey.ToCOLORREF();
		COLORREF SymbolColor = Colours::AircraftLightGrey.ToCOLORREF();

		CPen TrailPen(PS_SOLID, 1, TrailColor);
		CPen SymbolPen(PS_SOLID, 1, SymbolColor);

		dc->SelectObject(&TrailPen);
		dc->SelectStockObject(NULL_BRUSH);

		// History Trail
		CRadarTargetPositionData TrailPosition = radarTarget.GetPreviousPosition(radarTarget.GetPreviousPosition(radarTarget.GetPosition()));
		for (int i = 0; i < 5; i++) {
			if (!drawTrail)
				break;

			POINT historyTrailPoint = radar->ConvertCoordFromPositionToPixel(TrailPosition.GetPosition());

			CRect Area(
				historyTrailPoint.x - DRAWING_AC_SQUARE_TRAIL_SIZE,
				historyTrailPoint.y - DRAWING_AC_SQUARE_TRAIL_SIZE,
				historyTrailPoint.x + DRAWING_AC_SQUARE_TRAIL_SIZE,
				historyTrailPoint.y + DRAWING_AC_SQUARE_TRAIL_SIZE
			);
			Area.NormalizeRect();
			dc->Rectangle(Area);

			// Skip one position for the other
			TrailPosition = radarTarget.GetPreviousPosition(radarTarget.GetPreviousPosition(TrailPosition));
		}

		dc->SelectObject(&SymbolPen);

		// Symbol itself
		POINT radarTargetPoint = radar->ConvertCoordFromPositionToPixel(radarTarget.GetPosition().GetPosition());

		int DiamondSize = DRAWING_AC_PRIMARY_DIAMOND_SIZE;

		dc->MoveTo(radarTargetPoint.x, radarTargetPoint.y - DiamondSize);
		dc->LineTo(radarTargetPoint.x - DiamondSize, radarTargetPoint.y);
		dc->LineTo(radarTargetPoint.x, radarTargetPoint.y + DiamondSize);
		dc->LineTo(radarTargetPoint.x + DiamondSize, radarTargetPoint.y);
		dc->LineTo(radarTargetPoint.x, radarTargetPoint.y - DiamondSize);

		dc->RestoreDC(save);
		return CRect(
			radarTargetPoint.x - DiamondSize,
			radarTargetPoint.y - DiamondSize,
			radarTargetPoint.x + DiamondSize,
			radarTargetPoint.y + DiamondSize
		);
	}

	static void DrawSpeedVector(CDC* dc, TagConfiguration::TagStates State, CRadarScreen* radar, CRadarTarget radarTarget, bool isPrimary, bool isSoft, int Seconds) {
		int save = dc->SaveDC();
		
		COLORREF VectorColor = Colours::AircraftDarkGrey.ToCOLORREF();

		if (!isSoft)
			VectorColor = Colours::AircraftLightGrey.ToCOLORREF();

		if (State == TagConfiguration::TagStates::Assumed ||
			State == TagConfiguration::TagStates::Next ||
			State == TagConfiguration::TagStates::TransferredToMe)
			VectorColor = Colours::AircraftGreen.ToCOLORREF();

		CPen GreenPen(PS_SOLID, 1, VectorColor);
		dc->SelectObject(&GreenPen);

		POINT AcPoint = radar->ConvertCoordFromPositionToPixel(radarTarget.GetPosition().GetPosition());

		double d = double(radarTarget.GetPosition().GetReportedGS() * 0.514444) * Seconds;
		CPosition PredictedEnd = Extrapolate(radarTarget.GetPosition().GetPosition(), radarTarget.GetTrackHeading(), d);
		POINT PredictedEndPoint = radar->ConvertCoordFromPositionToPixel(PredictedEnd);

		int bound = DRAWING_AC_SQUARE_SYMBOL_SIZE + DRAWING_PADDING;

		if (isPrimary)
			bound = DRAWING_AC_PRIMARY_DIAMOND_SIZE + DRAWING_PADDING;

		CRect Area(
			AcPoint.x - bound,
			AcPoint.y - bound,
			AcPoint.x + bound,
			AcPoint.y + bound
		);

		POINT ClipFrom, ClipTo;
		if (LiangBarsky(Area, AcPoint, PredictedEndPoint, ClipFrom, ClipTo)) {
			dc->MoveTo(ClipTo.x, ClipTo.y);
			dc->LineTo(PredictedEndPoint.x, PredictedEndPoint.y);
		}

		dc->RestoreDC(save);
	}
	
	static CRect DrawApproachVector(CDC* dc, CRadarScreen* radar, CRadarTarget radarTarget, double distance = 3) {
		int save = dc->SaveDC();

		double reverseHeading = static_cast<int>(radarTarget.GetTrackHeading() + 180) % 360;
		CPosition middlePointArrow = Extrapolate(radarTarget.GetPosition().GetPosition(), reverseHeading, distance * 1852);

		double topArrowHeading = static_cast<int>(reverseHeading - 40) % 360;
		CPosition topPointArrow = Extrapolate(middlePointArrow, topArrowHeading, 0.6 * 1852);

		double bottomArrowHeading = static_cast<int>(reverseHeading + 40) % 360;
		CPosition bottomPointArrow = Extrapolate(middlePointArrow, bottomArrowHeading, 0.6 * 1852);
		
		CPen ArrowPen(PS_SOLID, 1, Colours::PurpleDisplay.ToCOLORREF());
		dc->SelectObject(&ArrowPen);

		POINT middleArrowPointPx = radar->ConvertCoordFromPositionToPixel(middlePointArrow);
		POINT topArrowPointPx = radar->ConvertCoordFromPositionToPixel(topPointArrow);
		POINT bottomArrowPointPx = radar->ConvertCoordFromPositionToPixel(bottomPointArrow);

		dc->MoveTo(middleArrowPointPx);
		dc->LineTo(topArrowPointPx);

		dc->MoveTo(middleArrowPointPx);
		dc->LineTo(bottomArrowPointPx);

		dc->RestoreDC(save);

		CRect r(topArrowPointPx.x, topArrowPointPx.y, middleArrowPointPx.x, bottomArrowPointPx.y);
		r.NormalizeRect();
		return r;
	};
};
