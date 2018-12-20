#pragma once

#include "debugterminal.h"
#include "server.h"
#include "camera.h"
#include "callbacks.h"
#include "datadecoder.h"
#include "fieldmap.h"
#include "battery.h"
#include "autodata.h"

#define sizeofControlSpace 100

#define PARENT_AUTOPILOT_TIMER 5

class ControlPanel : public ControlPanelCallback {
	bool init = false;
	DebugTerminal dt;
	FieldMap fm;
	HWND parent;
	HINSTANCE instance;
	Server server;
	Camera networkCameraFront;//0
	Battery battery;
	AutoData autoData;
	int autopilotEffectStage = 0;
	bool hasAutoData = false;//always flash anyways no matter what it says
public:
	bool getHasAutoData() {
		return hasAutoData;
	}
	DataDecodeSingleArgIntKey imageMap;
	DataDecodeSingleArgStringKey cameraSettings;
	DataDecodeSingleArgStringKey generalSettings;
	void setAutoData(int status) override {
		hasAutoData = status == 1;
		InvalidateRect(parent, NULL, FALSE);
	}
	void setBattery(float volt) override {
		battery.setBatteryPercentageVoltage( (volt-11.5f)/0.8f, volt);
	}
	void serverStatusUpdated() override {
		//hasAutoData = false;
		InvalidateRect(parent, NULL, FALSE);
	}
	void sendAutoData(int i1, int i2, int i3, int i4) override {
		MRCCommand com;
		com.setType4(MRCCommand::MRCC_AUTODATA, i1, i2, i3, i4);
		server.deliver(com);
	}
	void sendCamerasStatus() override {
		MRCCommand com;
		com.setType4(MRCCommand::MRCC_CAMERASTATUS, networkCameraFront.getid(), networkCameraFront.isConnected(), networkCameraFront.hasFrame(), 0);
		server.deliver(com);
	}
	void sendTrackingStatus() override {
		MRCCommand com;
		com.setType4(MRCCommand::MRCC_TRACKINGSTATUS, networkCameraFront.getid(), networkCameraFront.hasTrackingOrders(), networkCameraFront.isTracking(), 0);
		server.deliver(com);
	}
	void sendAABB(int camid) override {
		//Cam 0
		if (camid == networkCameraFront.getid()) {
			MRCCommand com;
			com.setType3(MRCCommand::MRCC_TRACKINGAABB, networkCameraFront.getid(), 0, networkCameraFront.getAABB().x, networkCameraFront.getAABB().y, networkCameraFront.getAABB().width, networkCameraFront.getAABB().height);
			server.deliver(com);
		}
	}
	void setAutoPilot(bool status) override {
		if (status)
			SetTimer(parent, PARENT_AUTOPILOT_TIMER, 500, NULL);
		else
			KillTimer(parent, PARENT_AUTOPILOT_TIMER);
		InvalidateRect(parent, NULL, FALSE);
	}
	void stopTracking(int camid) override {
		if (networkCameraFront.getid() == camid)
			networkCameraFront.resetTracker();
	}

	void trackImage(int camid, int imgid, int method, int trackmethod) override {
		if (networkCameraFront.getid() == camid) {
			if (networkCameraFront.trackImage(imgid, method, trackmethod, &imageMap.keys, &imageMap.values)) {
				MRCCommand com;
				com.setType4(MRCCommand::MRCC_TRACKRETURN, camid, 1, 0, 0);
				server.deliver(com);
			}
			else {
				MRCCommand com;
				com.setType4(MRCCommand::MRCC_TRACKRETURN, camid, 0, 0, 0);
				server.deliver(com);
			}
		}

	}
	void track(int camid, int trackmethod, int x, int y, int w, int h) override {
		if (networkCameraFront.getid() == camid) {
			if (networkCameraFront.track(trackmethod, x, y, w, h)) {
				MRCCommand com;
				com.setType4(MRCCommand::MRCC_TRACKRETURN, camid, 1, 0, 0);
				server.deliver(com);
			}
			else {
				MRCCommand com;
				com.setType4(MRCCommand::MRCC_TRACKRETURN, camid, 0, 0, 0);
				server.deliver(com); 
			}
		}

	}
	void showDebugConsole() override {
		dt.show();
	}
	void initialize(HWND _parent, HINSTANCE _instance) {
		if (init)
			return;


		imageMap.decodeFile("data/imagemap.txt");
		generalSettings.decodeFile("data/generalsettings.txt");
		cameraSettings.decodeFile("data/camerasettings.txt");

		
		
		RECT rec;
		GetClientRect(_parent, &rec);

		parent = _parent;
		instance = _instance;
		dt.initialize(parent, instance);
		fm.initialize(parent, instance);
		battery.initialize(parent, instance);
		autoData.initialize(parent, instance, this);
		string camera0set = cameraSettings.lookup("CAMERA0TYPE");
		if (camera0set == "LOCAL") {
			networkCameraFront.initialize(0, this, parent, instance, &dt, 20, 20, rec.right - 20, rec.bottom - sizeofControlSpace - 20,
				stoi(cameraSettings.lookup("CAMERA0LOCAL")), stoi(cameraSettings.lookup("CAMERA0MS")));
		}
		else if (camera0set == "NETWORK") {
			networkCameraFront.initialize(0, this, parent, instance, &dt, 20, 20, rec.right - 20, rec.bottom - sizeofControlSpace - 20, cameraSettings.lookup("CAMERA0IP"), cameraSettings.lookup("CAMERA0PORT"), getFile(cameraSettings.lookup("CAMERA0GET")));
		}
		server.initialize(&dt, this, generalSettings.lookup("SERVERPORT"));

		init = true;

		hasAutoData = false;
		InvalidateRect(parent,NULL,FALSE);
	}
	void shutdown() {
		if (!init)
			return;
		init = false;

		server.cease();
		networkCameraFront.cease();
		server.shutdown();
		networkCameraFront.shutdown();
		fm.shutdown();
		battery.shutdown();
		autoData.shutdown();
		dt.dump("data/debugterminaldump.txt");
		dt.shutdown();

	}
	bool WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {//return true if it handles it
		if (init)
		{
			switch (message) {
			case WM_TIMER:
				autopilotEffectStage++;
				if (autopilotEffectStage == 2)//Maybe just use a boolean... but thinking about using more than 2 colors
					autopilotEffectStage = 0;
				InvalidateRect(hWnd, NULL, FALSE);
				break;
			case WM_DEVICECHANGE:
			{
					networkCameraFront.usbChange(); 
			}
				break;
			case WM_SIZE:
			{
				int nWidth = LOWORD(lParam);
				int nHeight = HIWORD(lParam);
				networkCameraFront.resize(20, 20, nWidth - 20, nHeight - sizeofControlSpace - 20);
				return true;
			}break;
			case WM_COMMAND:
			{
				int wmId = LOWORD(wParam);
				switch (wmId)
				{
				case IDM_DEBUG:
					dt.show();
					break;
				case IDM_FIELDMAP:
					fm.show();
					break;
				case IDM_BATTERY:
					battery.show();
					break;
				case IDM_AUTODATA:
					autoData.show();
					break;
				}
			} break;
			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);

				//https://msdn.microsoft.com/en-us/library/ms969905 anti-flicker
				RECT rc2;
				HDC hdcMem2;
				HGDIOBJ hbmMem2, hbmOld2;
				HBRUSH hbrBkGnd2;
				GetClientRect(hWnd, &rc2);
				hdcMem2 = CreateCompatibleDC(ps.hdc);
				hbmMem2 = CreateCompatibleBitmap(ps.hdc, rc2.right - rc2.left, rc2.bottom - rc2.top);
				hbmOld2 = SelectObject(hdcMem2, hbmMem2);
				hbrBkGnd2 = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
				FillRect(hdcMem2, &rc2, hbrBkGnd2);
				DeleteObject(hbrBkGnd2);

				RECT rec;
				GetClientRect(hWnd, &rec);

				HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
				HBRUSH panelGray = CreateSolidBrush(RGB(50, 50, 50));
				HBRUSH borderGray = CreateSolidBrush(RGB(100, 100, 100));
				HBRUSH red = CreateSolidBrush(RGB(255, 0, 0));
				HBRUSH green = CreateSolidBrush(RGB(0, 255, 0));

				GRADIENT_RECT gradientRect = { 0, 1 };
				COLORREF rgbTop = RGB(100, 100, 100);
				COLORREF rgbBottom = RGB(0, 0, 0);
				TRIVERTEX triVertext[2] = {
					rec.left - 1,
					rec.top - 1,
					GetRValue(rgbTop) << 8,
					GetGValue(rgbTop) << 8,
					GetBValue(rgbTop) << 8,
					0x0000,
					rec.right,
					rec.bottom - sizeofControlSpace,
					GetRValue(rgbBottom) << 8,
					GetGValue(rgbBottom) << 8,
					GetBValue(rgbBottom) << 8,
					0x0000
				};
				GradientFill(hdcMem2, triVertext, 2, &gradientRect, 1, GRADIENT_FILL_RECT_V);

				RECT panel = rec;
				panel.top = panel.bottom - sizeofControlSpace;
				FillRect(hdcMem2, &panel, panelGray);
				panel.bottom = panel.bottom - sizeofControlSpace + 2;
				FillRect(hdcMem2, &panel, borderGray);
				panel.bottom = rec.bottom;
				panel.top = panel.top + 2;

				if (server.isConnectedToRobot())
				{
					RECT statRec;
					statRec.left = 5;
					statRec.top = panel.top + 5;
					statRec.right = statRec.left + 2;
					statRec.bottom = statRec.top + 20;
					FillRect(hdcMem2, &statRec, green);
				}
				else
				{
					RECT statRec;
					statRec.left = 5;
					statRec.top = panel.top + 5;
					statRec.right = statRec.left + 2;
					statRec.bottom = statRec.top + 20;
					FillRect(hdcMem2, &statRec, red);
				}

				if (server.isConnectedToRobot())
				{
					if (server.getAutoPilotStatus())
					{
						RECT statRec;
						statRec.left = 5;
						statRec.top = panel.top + 5 + 20 + 3;
						statRec.right = statRec.left + 2;
						statRec.bottom = statRec.top + 20;
						FillRect(hdcMem2, &statRec, green);
					}
					else
					{
						RECT statRec;
						statRec.left = 5;
						statRec.top = panel.top + 5 + 20 + 3;
						statRec.right = statRec.left + 2;
						statRec.bottom = statRec.top + 20;
						FillRect(hdcMem2, &statRec, red);
					}

					if (getHasAutoData())
					{
						RECT statRec;
						statRec.left = 5;
						statRec.top = panel.top +3+20+ 5 + 20 + 3;
						statRec.right = statRec.left + 2;
						statRec.bottom = statRec.top + 20;
						FillRect(hdcMem2, &statRec, green);
					}
					else
					{
						RECT statRec;
						statRec.left = 5;
						statRec.top = panel.top + 3 + 20 + 5 + 20 + 3;
						statRec.right = statRec.left + 2;
						statRec.bottom = statRec.top + 20;
						FillRect(hdcMem2, &statRec, red);
					}
				}

				//Text
				SetBkMode(hdcMem2, TRANSPARENT);

				std::wstring str = L"2973";
				SetTextAlign(hdcMem2, TA_RIGHT);
				SetTextColor(hdcMem2, RGB(255, 255, 255));
				HFONT hFont = CreateFont(sizeofControlSpace - 2, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, L"SYSTEM_FIXED_FONT");
				HFONT hTmp = (HFONT)SelectObject(hdcMem2, hFont);
				TextOut(hdcMem2, rec.right - 5, rec.bottom - sizeofControlSpace + 2, str.c_str(), str.size());
				DeleteObject(SelectObject(hdcMem2, hTmp));

				if (server.getAutoPilotStatus())
				{
					hFont = CreateFont(40, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, L"SYSTEM_FIXED_FONT");
					hTmp = (HFONT)SelectObject(hdcMem2, hFont);
					str = L"AUTOPILOT ENABLED";
					SetTextAlign(hdcMem2, TA_CENTER);
					if (autopilotEffectStage == 0)
						SetTextColor(hdcMem2, RGB(255, 255, 255));
					else
						SetTextColor(hdcMem2, RGB(255, 0, 0, ));
					TextOut(hdcMem2, panel.right/2, panel.top + 2, str.c_str(), str.size());
					DeleteObject(SelectObject(hdcMem2, hTmp));
				}

				hFont = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, L"SYSTEM_FIXED_FONT");
				hTmp = (HFONT)SelectObject(hdcMem2, hFont);

				str = server.getStatusWString();
				SetTextAlign(hdcMem2, TA_LEFT);
				SetTextColor(hdcMem2, RGB(255, 255, 255));
				TextOut(hdcMem2, 10, panel.top + 5, str.c_str(), str.size());

				if (server.isConnectedToRobot())
				{
					str = server.getAutoPilotStatus() ? L"Autopilot Enabled" : L"Autopilot Disabled";
					SetTextAlign(hdcMem2, TA_LEFT);
					SetTextColor(hdcMem2, RGB(255, 255, 255));
					TextOut(hdcMem2, 10, panel.top + 5 + 20 + 3, str.c_str(), str.size());

					str = getHasAutoData() ? L"Autonomous Flashed" : L"Autonomous Not Flashed";
					SetTextAlign(hdcMem2, TA_LEFT);
					SetTextColor(hdcMem2, RGB(255, 255, 255));
					TextOut(hdcMem2, 10, panel.top +3 +20+ 5 + 20 + 3, str.c_str(), str.size());
				}

				if (getHasAutoData()) {
					str = server.getAutoPilotStatus() ? L"Autopilot Enabled" : L"Autopilot Disabled";
					SetTextAlign(hdcMem2, TA_LEFT);
					SetTextColor(hdcMem2, RGB(255, 255, 255));
					TextOut(hdcMem2, 10, panel.top + 5 + 20 + 3, str.c_str(), str.size());

				}

				DeleteObject(SelectObject(hdcMem2, hTmp));

				SetBkMode(hdcMem2, OPAQUE);

				DeleteObject(black);
				DeleteObject(panelGray);
				DeleteObject(borderGray);
				DeleteObject(red);
				DeleteObject(green);


				//https://msdn.microsoft.com/en-us/library/ms969905 anti-flicker
				BitBlt(ps.hdc, rc2.left, rc2.top, rc2.right - rc2.left, rc2.bottom - rc2.top,
					hdcMem2,
					0,0,
					SRCCOPY
					);
				SelectObject(hdcMem2, hbmOld2);
				DeleteObject(hbmMem2);
				DeleteDC(hdcMem2);

				EndPaint(hWnd, &ps);
				return true;
			}
			break;
			}
		}
		return false;
	}
};