#pragma once

#include <string>
#include <Windows.h>
#include <tchar.h>  
#include <fstream>
#include <locale>
#include <codecvt>

//add class inheritence to tools


#include "callbacks.h"

LRESULT CALLBACK Slider_WndProc(
	_In_ HWND   hwnd,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
);

class Slider {
	bool init = false;
	HWND parent;
	HINSTANCE instance;
	HWND thisWindow;
	WNDCLASSEX wc;
	bool makeClass() {

		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = 0;
		wc.lpfnWndProc = Slider_WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = L"MR_Slider";
		wc.hInstance = instance;
		wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

		if (!RegisterClassEx(&wc))
			return false;
		return true;
	}
	HWND gui_button_send = NULL;
	HWND gui_tb_1 = NULL;
	HWND gui_tb_2 = NULL;
	HWND gui_tb_3 = NULL;
	HWND gui_tb_4 = NULL;

	bool makeWindow() {

		RECT win;
		win.left = 0;
		win.right = 200;
		win.top = 0;
		win.bottom = 120;
		AdjustWindowRect(&win, WS_CAPTION | WS_THICKFRAME, false);
		thisWindow = CreateWindow(L"MR_Slider", L"Slider", WS_OVERLAPPED | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, win.right, win.bottom, parent, NULL, instance, NULL);
		if (!thisWindow)
			return false;
		SetWindowLongPtr(thisWindow, GWLP_USERDATA, (LONG_PTR)this);
		RECT finalWindowRect;
		GetClientRect(thisWindow, &finalWindowRect);
		gui_button_send = CreateWindow(L"button", L"Flash", WS_VISIBLE | WS_CHILD, finalWindowRect.right / 2 - 150 / 2, finalWindowRect.bottom - 40 - 5, 150, 40, thisWindow, (HMENU)0, instance, NULL);
		int wid = (finalWindowRect.right - finalWindowRect.left - 5 * 5) / 4;
		int hid = finalWindowRect.bottom - 5 - 5 - 5 - 40;
		gui_tb_1 = CreateWindow(L"edit", L"0", WS_TABSTOP | WS_BORDER | ES_NUMBER | WS_VISIBLE | WS_CHILD, 5 + 0 * (wid + 5), 5, wid, hid, thisWindow, (HMENU)0, instance, NULL);
		gui_tb_2 = CreateWindow(L"edit", L"0", WS_TABSTOP | WS_BORDER | ES_NUMBER | WS_VISIBLE | WS_CHILD, 5 + 1 * (wid + 5), 5, wid, hid, thisWindow, (HMENU)0, instance, NULL);
		gui_tb_3 = CreateWindow(L"edit", L"0", WS_TABSTOP | WS_BORDER | ES_NUMBER | WS_VISIBLE | WS_CHILD, 5 + 2 * (wid + 5), 5, wid, hid, thisWindow, (HMENU)0, instance, NULL);
		gui_tb_4 = CreateWindow(L"edit", L"0", WS_TABSTOP | WS_BORDER | ES_NUMBER | WS_VISIBLE | WS_CHILD, 5 + 3 * (wid + 5), 5, wid, hid, thisWindow, (HMENU)0, instance, NULL);

		render();

		return true;
	}
	void render() {
		if (!init)
			return;
		InvalidateRect(thisWindow, NULL, FALSE);
	}
	ControlPanelCallback* cb;
public:
	void initialize(HWND _parent, HINSTANCE _instancem, ControlPanelCallback* _cb) {
		if (init)
			return;
		cb = _cb;
		parent = _parent;
		instance = _instancem;
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
	void shutdown() {
		if (!init)
			return;
		init = false;
		if (gui_button_send) {
			DestroyWindow(gui_button_send);
			gui_button_send = NULL;
		}
		if (gui_tb_1) {
			DestroyWindow(gui_tb_1);
			gui_tb_1 = NULL;
		}
		if (gui_tb_2) {
			DestroyWindow(gui_tb_2);
			gui_tb_2 = NULL;
		}
		if (gui_tb_3) {
			DestroyWindow(gui_tb_3);
			gui_tb_3 = NULL;
		}
		if (gui_tb_4) {
			DestroyWindow(gui_tb_4);
			gui_tb_4 = NULL;
		}
		//Window close
		DestroyWindow(thisWindow);

	}
	~Slider() {
		shutdown();
	}

	friend LRESULT CALLBACK Slider_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

LRESULT CALLBACK Slider_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Slider* thisAutoData = (Slider*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	PAINTSTRUCT ps;
	HDC hdc;
	switch (message)
	{
	case WM_PAINT: {
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps); }
				   break;
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		return 1;
		break;
	case WM_DESTROY:

		break;
	case WM_COMMAND: {
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		}
	}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}