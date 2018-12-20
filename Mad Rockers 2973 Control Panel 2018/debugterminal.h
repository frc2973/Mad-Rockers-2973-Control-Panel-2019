#pragma once

#include <string>
#include <Windows.h>
#include <tchar.h>  
#include <fstream>
#include <locale>
#include <codecvt>
#include <chrono>


LRESULT CALLBACK DebugTerminal_WndProc(
	_In_ HWND   hwnd,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
);

class DebugTerminal {
	bool init = false;
	std::wstring outputWindowContents = L"";
	HWND parent;
	HWND tb;
	HINSTANCE instance;
	HWND thisWindow;
	WNDCLASSEX wc;
	//may add vector of inputs in the future
	bool makeClass() {

		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = 0;
		wc.lpfnWndProc = DebugTerminal_WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = L"MR_DEBUGTERMINAL";
		wc.hInstance = instance;
		wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

		if (!RegisterClassEx(&wc))
			return false;
		return true;
	}
	bool makeWindow() {
		thisWindow = CreateWindow(L"MR_DEBUGTERMINAL", L"Debug Terminal", WS_OVERLAPPED | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 500, 500+20, parent, NULL, instance, NULL);
		if (!thisWindow)
			return false;

		SetWindowLongPtr(thisWindow, GWLP_USERDATA, (LONG_PTR)this);

		tb = CreateWindowEx(0, L"EDIT", 0, WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_LEFT|ES_MULTILINE|ES_AUTOVSCROLL, 0,0,400,400, thisWindow, NULL, instance, NULL);

		ShowScrollBar(tb, SB_BOTH, TRUE);
		RECT rec;
		GetClientRect(thisWindow, &rec);
		SetWindowPos(tb,NULL,0,0,rec.right,rec.bottom-20,SWP_NOMOVE);

		render();

		return true;
	}
	void render() {
		SendMessage(tb, WM_SETTEXT, 0, (LPARAM)outputWindowContents.c_str());
		SendMessage(tb, EM_SETSEL, 0, -1);
		SendMessage(tb, EM_SETSEL, -1, -1);
		SendMessage(tb, EM_SCROLLCARET, 0, 0);
	}
	void renderActual() {
		InvalidateRect(thisWindow, NULL, true);
	}
	std::wstring additionalOutput = L"";
	void checkOutputSize() {
		if (outputWindowContents.length() > 10000)
		{
			additionalOutput += outputWindowContents;
			outputWindowContents = L"Terminal exceeded the maximum length and has been cleared. Dump will contain the full log.\r\n";
		}
	}
public: //Most error checking in here
	//Prints a line
	DebugTerminal& operator<< (std::string rhand) {
		if (init) {
			std::wstring ws;
			ws.assign(rhand.begin(), rhand.end());
			outputWindowContents += ws + L"\r\n";
			checkOutputSize();
			render();
		}
		return *this;
	}
	DebugTerminal& operator<< (std::wstring rhand) {
		if (init) {
			outputWindowContents += rhand + L"\r\n";
			checkOutputSize();
			render();
		}
		return *this;
	}
	void initialize(HWND _parent, HINSTANCE _instance) {
		if (init)
			return;

		parent = _parent;
		instance = _instance; 
		additionalOutput = L"";
		outputWindowContents = L"Mad Rockers 2973 Control Panel 2018\r\nMade for the 2018 competition year.\r\nMiddle-click camera windows to show color map calibration. Again to save.\r\n";
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
	void dump(std::string path) {
		std::ofstream os(path); 
		using convert_type = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_type, wchar_t> converter;
		os<<(converter.to_bytes(additionalOutput+outputWindowContents));
		os.close();
	}
	void shutdown() {
		if (!init)
			return;
		init = false;

		//Window close
		DestroyWindow(tb);
		DestroyWindow(thisWindow);

		outputWindowContents = L"";
		additionalOutput = L"";
	}
	~DebugTerminal() {
		shutdown();
	}

	int rps = 0, sps = 0, crps = 0, csps = 0;
	std::chrono::milliseconds msRPS = (std::chrono::milliseconds)0;
	void setNetInfo(int _changedrps, int _changedsps) {
		crps += _changedrps;
		csps += _changedsps;
		std::chrono::milliseconds cur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
		if (msRPS == (std::chrono::milliseconds)0)
			msRPS = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
		if (cur - msRPS > (std::chrono::milliseconds)1000) {
			rps = crps;
			sps = csps;
			crps = csps = 0;
			renderActual();
			msRPS = cur;
		}
	}

	std::wstring getBottomString() {
		std::wstring response = L"RPS: " + std::to_wstring(rps) + L", SPS: " + std::to_wstring(sps);
		return response;
	}
};

LRESULT CALLBACK DebugTerminal_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DebugTerminal* thisDebugTerminal = (DebugTerminal*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	PAINTSTRUCT ps;
	HDC hdc;
	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		if (thisDebugTerminal) {
			RECT clientRect;
			GetClientRect(hWnd, &clientRect);

			SetBkMode(hdc, TRANSPARENT);
			std::wstring str = thisDebugTerminal->getBottomString();
			SetTextAlign(hdc, TA_LEFT);
			SetTextColor(hdc, RGB(0, 0, 0));
			HFONT hFont = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, L"SYSTEM_FIXED_FONT");
			HFONT hTmp = (HFONT)SelectObject(hdc, hFont);
			TextOut(hdc, 5, clientRect.bottom-20, str.c_str(), str.size());
			DeleteObject(SelectObject(hdc, hTmp));
		}
		EndPaint(hWnd, &ps);
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