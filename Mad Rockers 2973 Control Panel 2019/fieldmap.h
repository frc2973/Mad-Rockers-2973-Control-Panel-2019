#pragma once

#include <string>
#include <Windows.h>
#include <tchar.h>  
#include <fstream>
#include <locale>
#include <codecvt>

//add class inheritence to tools

LRESULT CALLBACK FieldMap_WndProc(
	_In_ HWND   hwnd,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
);

class FieldMap {
	bool init = false;
	HWND parent;
	HINSTANCE instance;
	HWND thisWindow;
	WNDCLASSEX wc;
	//may add vector of inputs in the future
	bool makeClass() {

		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = 0;
		wc.lpfnWndProc = FieldMap_WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = L"MR_FIELDMAP";
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
		win.bottom = 100;
		AdjustWindowRect(&win, WS_CAPTION | WS_THICKFRAME,false);
		thisWindow = CreateWindow(L"MR_FIELDMAP", L"Field Map", WS_OVERLAPPED | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, win.right, win.bottom, parent, NULL, instance, NULL);
		if (!thisWindow)
			return false;
		SetWindowLongPtr(thisWindow, GWLP_USERDATA, (LONG_PTR)this);

		render();

		return true;
	}
public:
	void render() {
		if (!init)
			return;
		InvalidateRect(thisWindow, NULL, FALSE);
	}
	void initialize(HWND _parent, HINSTANCE _instance) {
		if (init)
			return;

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
	void shutdown() {
		if (!init)
			return;
		init = false;
		//Window close
		DestroyWindow(thisWindow);

	}
	~FieldMap() {
		shutdown();
	}
};

LRESULT CALLBACK FieldMap_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	FieldMap* thisFieldMap = (FieldMap*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	PAINTSTRUCT ps;
	HDC hdc;
	switch (message)
	{
	case WM_PAINT: {
		hdc = BeginPaint(hWnd, &ps);
		HBRUSH panelGray = CreateSolidBrush(RGB(50, 50, 50));
		HBRUSH red = CreateSolidBrush(RGB(255, 0, 0));
		HBRUSH blue = CreateSolidBrush(RGB(0, 0, 255));
		RECT genRect;
		GetClientRect(hWnd, &genRect);
		FillRect(hdc, &genRect, panelGray);
		RECT rRect = { 5,5,5+20,genRect.bottom - 5 };
		FillRect(hdc, &rRect, red);
		rRect = { genRect.right - 25,5,genRect.right - 5,genRect.bottom - 5 };
		FillRect(hdc, &rRect, blue);
		TextOut(hdc, 12, 12, L"Error: Not Implemented.", 24);
		/*
		if (!thisFieldMap)
			TextOut(hdc,24,2,L"Not Linked", 11);
		else {
			if (thisFieldMap->hasData())
			{
				TextOut(hdc, 24, 2, L"Has Data", 8);
			}
			else
			{
				TextOut(hdc, 24, 2, L"No Data", 8);
			}
		}*/
		DeleteObject(panelGray); DeleteObject(red); DeleteObject(blue);
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