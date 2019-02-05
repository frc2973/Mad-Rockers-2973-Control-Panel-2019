#pragma once

#include <string>
#include <Windows.h>
#include <tchar.h>  
#include <fstream>
#include <locale>
#include <codecvt>
#include <ctime>
#include <vector>
#include <algorithm>

#include "callbacks.h"

//add class inheritence to tools

//this file frequently changes year to year

LRESULT CALLBACK Display_WndProc(
	_In_ HWND   hwnd,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
);

class Display {
	bool init = false;
	HWND parent;
	HINSTANCE instance;
	ControlPanelCallback* cb;
	HWND thisWindow;
	WNDCLASSEX wc;
	int i1 =-1, i2=-1, i3=-1, i4=-1;
	bool makeClass() {

		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = 0;
		wc.lpfnWndProc = Display_WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = L"MR_Display";
		wc.hInstance = instance;
		wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

		if (!RegisterClassEx(&wc))
			return false;
		return true;
	}
	bool makeWindow() {

		RECT win;
		win.left = 0;
		win.right = 200;
		win.top = 0;
		win.bottom = 334;
		AdjustWindowRect(&win, WS_CAPTION | WS_THICKFRAME, false);
		thisWindow = CreateWindow(L"MR_Display", L"Display", WS_OVERLAPPED | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, win.right, win.bottom, parent, NULL, instance, NULL);
		if (!thisWindow)
			return false;
		SetWindowLongPtr(thisWindow, GWLP_USERDATA, (LONG_PTR)this);

		render();

		return true;
	}
	void render() {
		if (!init)
			return;
		InvalidateRect(thisWindow, NULL, FALSE);
	}
public:
	HWND getHWND() {
		return thisWindow;
	}
	HBRUSH rocket1 = nullptr;
	void initialize(HWND _parent, HINSTANCE _instance, ControlPanelCallback* _cb) {
		if (init)
			return;

		cb = _cb;

		parent = _parent;
		instance = _instance;
		if (!makeClass())
			return;
		if (!makeWindow())
			return;

		
		init = true;



	}
	void show() {
		if (init) {
			ShowWindow(thisWindow, SW_SHOW);
			RECT rec;
			GetWindowRect(parent, &rec);
			SetWindowPos(thisWindow, NULL, rec.left + 10, rec.top + 10, 0, 0, SWP_NOSIZE);
			UpdateWindow(thisWindow);
		}
	}
	bool hasData() {
		if (!init)
			return false;
		return false;
	}
	void setDisplay(int _i1, int _i2, int _i3, int _i4) {
		i1 = _i1;//For this comp, will i1 be for lift?
		i2 = _i2;
		i3 = _i3;
		i4 = _i4;
		render();
	}
	void shutdown() {
		if (!init)
			return;
		init = false;
		//Window close
		DestroyWindow(thisWindow);
		if (rocket1 != nullptr) {
			DeleteObject(rocket1);
			rocket1 = nullptr;
		}

	}
	~Display() {
		shutdown();
	}

	friend LRESULT CALLBACK Display_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

LRESULT CALLBACK Display_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	Display* thisDisplay = (Display*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	PAINTSTRUCT ps;
	HDC hdc;
	switch (message) {
	case WM_PAINT: {
		hdc = BeginPaint(hWnd, &ps);
		HBRUSH panelGray = CreateSolidBrush(RGB(50, 50, 50));
		HBRUSH lightGray = CreateSolidBrush(RGB(100, 100, 100));
		HBRUSH red = CreateSolidBrush(RGB(255, 0, 0));
		HBRUSH green = CreateSolidBrush(RGB(0, 255, 0));
		HBRUSH orange = CreateSolidBrush(RGB(255, 100, 0));
		RECT genRect;
		GetClientRect(hWnd, &genRect);
		FillRect(hdc, &genRect, panelGray);
		RECT rRect = { 5,5,genRect.right - 5,genRect.bottom - 5 };
		FillRect(hdc, &rRect, lightGray);


		if (thisDisplay->rocket1 == nullptr) {
			thisDisplay->rocket1 = CreatePatternBrush((HBITMAP)LoadImage(NULL, L"data/images/rocket1.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE));
		}

		rRect = {genRect.left,genRect.top,genRect.right,genRect.bottom};
		FillRect(hdc, &rRect, thisDisplay->rocket1);


		if (thisDisplay->i3 > 0) {
			std::wstring str3;
			rRect = { 160,0,200,150 };
			FillRect(hdc, &rRect, orange);
			str3 = L"CONTACT";
			TextOut(hdc, 12, 30, str3.c_str(), str3.size());
		}

		std::wstring str;
		str = to_wstring(thisDisplay->i1)
			+ L" " +to_wstring(thisDisplay->i2)
			+ L" " +to_wstring(thisDisplay->i3)
			+ L" " +to_wstring(thisDisplay->i4);
		TextOut(hdc, 12, 12, str.c_str(), str.size());

		


		if (thisDisplay->i1!=-1) {
			if (thisDisplay->i1 == 0)
				rRect = { 80,100,200,150 };
			else if (thisDisplay->i1 == 90)
				rRect = { 80,180,200,230 };
			else if (thisDisplay->i1 == 180)
				rRect = { 80,250,200,300 };
			else
				rRect = {0,100,100,300};
			FillRect(hdc, &rRect, green);
		}

		DeleteObject(orange);
		DeleteObject(panelGray); DeleteObject(red); DeleteObject(green); DeleteObject(lightGray);
		EndPaint(hWnd, &ps); }
				   break;
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		return 1;
		break;
	case WM_DESTROY:

		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}