#pragma once

#include "imagesearch.h"
#include "imagetracker.h"
#include "debugterminal.h"
#include "datadecoder.h"

#include <Windows.h>

//Mad rockers 2973 camera and image processing
//To be used for each camera stream, supports local and network cameras

LRESULT CALLBACK Camera_WndProc(
	_In_ HWND   hwnd,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
);

#include <windowsx.h>

#include <vector>
#include <thread>
#include <exception>

#include <opencv2\video.hpp>
#include <opencv2\videoio.hpp>
#include <opencv2\opencv.hpp>
#include "opencv2/highgui/highgui.hpp"

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
using boost::asio::ip::tcp;
using namespace std;

enum CameraType {
	CAMERA_NONE, CAMERA_NETWORK, CAMERA_LOCAL
};
//REORGANIZE THIS IT IS MESSY
class Camera {
	bool colorMapCalibration = false;

	int cilh, cihh, cils, cihs, cilv, cihv, cminr;
	void MUP() {
		
		if (colorMapCalibration)
		{
			colorMapCalibration = false;
			//Save to file
			ofstream ostr(colorMapURL);
			if (!ostr.is_open()) {
				if (output)
					*output << "Failed to open color map url to save data: " + colorMapURL;
			}
			else {
				ostr << "MINSIZE " << cminr << endl;
				ostr << "ILH " << cilh << endl;
				ostr << "IHH " << cihh << endl;
				ostr << "ILS " << cils << endl;
				ostr << "IHS " << cihs << endl;
				ostr << "ILV " << cilv << endl;
				ostr << "IHV " << cihv << endl;
			}
			ostr.close();
			//Hide debug windows
			using namespace cv;
			using namespace std;
			cvDestroyWindow((string("Color Map Calibration Control ")+to_string(id)).c_str());
			cvDestroyWindow((string("Color Map Calibration Thresholded Image ") + to_string(id)).c_str());
			if (output)
				*output << "Saving color map calibration data and hiding color map calibration tools for camera id " + to_string(id);
			waitKey(1);
		}
		else {
			//Load from file
			DataDecodeSingleArgStringKey colormap;
			colormap.decodeFile(colorMapURL);//Add advanced selection later
			if (colormap.keys.size() <= 0)
			{
				if (output)
					*output << "Image calibration failed. Invalid colormap.txt.";
				return;
			}
			cilh = stoi(colormap.lookup("ILH"));
			cihh = stoi(colormap.lookup("IHH"));
			cils = stoi(colormap.lookup("ILS"));
			cihs = stoi(colormap.lookup("IHS"));
			cilv = stoi(colormap.lookup("ILV"));
			cihv = stoi(colormap.lookup("IHV"));
			cminr = stoi(colormap.lookup("MINSIZE"));

			using namespace cv;
			using namespace std;
			string tcmcc = (string("Color Map Calibration Control ") + to_string(id));
			namedWindow(tcmcc.c_str(), CV_WINDOW_AUTOSIZE);
			namedWindow((string("Color Map Calibration Thresholded Image ") + to_string(id)).c_str(), CV_WINDOW_AUTOSIZE);
			cvCreateTrackbar("ILH", tcmcc.c_str(), &cilh, 179);
			cvCreateTrackbar("IHH", tcmcc.c_str(), &cihh, 179);
			cvCreateTrackbar("ILS", tcmcc.c_str(), &cils, 255);
			cvCreateTrackbar("IHS", tcmcc.c_str(), &cihs, 255);
			cvCreateTrackbar("ILV", tcmcc.c_str(), &cilv, 255);
			cvCreateTrackbar("IHV", tcmcc.c_str(), &cihv, 255);
			if (output)
				*output << "Showing color map calibration tools for camera id "+to_string(id);
			colorMapCalibration = true;
		}
	}
	
	void RCMC() {//RENDER
		if (!frameReady)
			return;
		if (!colorMapCalibration)
			return;

		using namespace cv;
		using namespace std;
		Mat imgHSV, imgThresholded;
		cvtColor(rawCamera, imgHSV, COLOR_BGR2HSV);
		inRange(imgHSV, Scalar(cilh, cils, cilv), Scalar(cihh, cihs, cihv), imgThresholded);
		erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		imshow((string("Color Map Calibration Thresholded Image ") + to_string(id)).c_str(), imgThresholded);
		waitKey(1);
	}
	bool frameReady = false;//If there is a bitmap stored to be rendered
	bool init = false;
	bool terminateThreads = false;
	bool connected = false;
	boost::asio::io_service io_service;
	tcp::socket socket;
	RECT windowLocation;
	HWND parent; HINSTANCE instance;
	HWND thisWindow;
	WNDCLASSEX wc;
	cv::Mat rawCamera; HBITMAP renHBITMAP = NULL;
	void makeWindow() {

		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = 0;
		wc.lpfnWndProc = Camera_WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(NULL);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = L"MR_NETCAML";
		wc.hInstance = instance;
		wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

		RegisterClassEx(&wc);

		thisWindow = CreateWindow(L"MR_NETCAML", 0, WS_CHILD | WS_VISIBLE, windowLocation.left, windowLocation.top, windowLocation.right - windowLocation.left, windowLocation.bottom - windowLocation.top, parent, NULL, instance, NULL);
		if (thisWindow)
			SetWindowLongPtr(thisWindow, GWLP_USERDATA, (LONG_PTR)this);
	}
	CameraType cameraType = CAMERA_NONE;
	int localCameraIndex; int timeBetweenFrames = 0;
	std::string ip; std::string port; std::string getRequest;
	std::thread* cameraProcessingThread;
	cv::VideoCapture cap;
	void setConnectStatus(bool st) {
		if (connected == st)//Allow for redundancy
			return;
		connected = st;
		if (output)
			*output << L"[CAMERA "+ getTypeWString() +L"] Connection status updated: "+(connected?L"CONNECTED":L"NOT CONNECTED");
		InvalidateRect(thisWindow, NULL, FALSE);
		if (connected) {
			untrustworthy_secstart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			actualFPS = -1;
			framesProcessed = 0;
		}
		if (connected) {
			if (sb_Stop)
				ShowWindow(sb_Stop, SW_SHOW);
		}
		else {
			if (sb_Stop)
				ShowWindow(sb_Stop, SW_HIDE);
		}
		if (parentcontrol)
			parentcontrol->sendCamerasStatus();
		//OutputDebugString(L"here");
	}
	void setFrameStatus(bool st) {
		if (frameReady == st)//Allow for redundancy
			return;
		frameReady = st;
		specResize();
		InvalidateRect(thisWindow, NULL, FALSE);
		if (parentcontrol)
			parentcontrol->sendCamerasStatus();
	}
	long long untrustworthy_secstart;
	int framesProcessed = 0;
	int actualFPS = -1;
	void enterASYNCNETSERVICE() {
		if (init&&connected) {
			try {
				boost::system::error_code err;
				boost::asio::write(socket, boost::asio::buffer(getRequest), err);
				if (err == boost::asio::error::eof)
				{
					throw("EOF, bad request.");
				}
				else if (err)
					throw boost::system::system_error(err);

				std::string jpgimg = "";
				char buf[1025];


				while(!terminateThreads) {
					buf[1024] = '\0';

					size_t len = socket.read_some(boost::asio::buffer(buf, 1024), err);
					

					if (err == boost::asio::error::eof)
						throw("EOF in loop");
					else if (err)
						throw boost::system::system_error(err);

					jpgimg.append(buf, buf + len);

					int a = jpgimg.find("\xff\xd8"), b = jpgimg.find("\xff\xd9");

					if (a != -1 && b != -1) {
						rawCamera = cv::imdecode(cv::Mat(1, b - a + 2, CV_8UC1, (void*)(&jpgimg[a])), CV_LOAD_IMAGE_COLOR);
						jpgimg = jpgimg.substr(b + 2);
						if (!frameReady) {
							setFrameStatus(true);
						}
						procRaw();
					}

				}
			}
			catch (std::exception& e) {
				if (output)
					*output << L"[CAMERA " + getTypeWString() + L"] Exception:";
				if (output)
					*output << e.what();
			}
			catch (boost::exception& e) {
				if (output)
					*output << L"[CAMERA " + getTypeWString() + L"] Exception:";
				if (output)
					*output << L"Boost exception";
			}
			catch (char* e) {
				if (output)
					*output << L"[CAMERA " + getTypeWString() + L"] Exception:";
				if (output)
					*output << std::string(e);
			}
			setFrameStatus(false);
			setConnectStatus(false);
		}
	}

	void specResize() {
		if (hasFrame()) {
			SetWindowPos(thisWindow, NULL, (windowLocation.right + windowLocation.left)/2 - (getMatToDisplay().cols)/2, (windowLocation.bottom + windowLocation.top) / 2 - (getMatToDisplay().rows) / 2, getMatToDisplay().cols, getMatToDisplay().rows, NULL);
		}else{
			SetWindowPos(thisWindow, NULL, windowLocation.left, windowLocation.top, windowLocation.right - windowLocation.left, windowLocation.bottom - windowLocation.top, NULL);
		}
	}
	void tryConnectUSB() {
		if (init)
		if ( !cap.isOpened() && cameraType == CAMERA_LOCAL) {
			try {
				if (cap.open(localCameraIndex))//Error
				{
					while (!cap.grab()) {}
					setConnectStatus(true);
				}
			}
			catch (std::exception e) {
				if (output) {
					*output << string("Camera exception caught on USB camera connection: ") + e.what();
					setConnectStatus(false);
				}
			}
		}
	}
	void checkUSB() {
		if (init) {
			if (cap.isOpened())
			{
				if (cap.grab())
				{
					return;
				}
				else
				{
					setFrameStatus(false);
					setConnectStatus(false);
					cap.release();
					return;
				}
			} else {
				setFrameStatus(false);
				setConnectStatus(false);
				cap.release();

			}
		}
	}
	void cameraPT() {
		if (!init)
			return;
		if (cameraType == CAMERA_LOCAL) {
			//Try connecting (remember there is a memory issue)
			tryConnectUSB();
			while (!terminateThreads) {
				//Check if the camera is connected
				if (isConnected()) {
					
					bool readError;
					try {
						readError = cap.read(rawCamera);
					}
					catch (std::exception e) {
						if (output) {
							*output << string("Camera exception caught on USB camera read: ") + e.what();
							readError = false;
						}
					}
					if (!readError)//Error
					{
						setFrameStatus(false);
						setConnectStatus(false);
						cap.release();
					}
					else if (rawCamera.cols>0&&rawCamera.rows>0)
					{
						if (!frameReady)
							setFrameStatus(true);

						procRaw();

						Sleep(timeBetweenFrames);
					}
				}
				else {
					Sleep(1000);//Wait for the USB camera to be connected
				}
			}
		}
		else if (cameraType == CAMERA_NETWORK) {

			while (!terminateThreads) {
				try {
					tcp::resolver resolver(io_service);
					tcp::resolver::query query(ip, port);
					tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
					socket = tcp::socket(io_service);
					boost::asio::connect(socket, endpoint_iterator);
					if (!socket.is_open())
						throw("Socket wasn't open.");

					setConnectStatus(true);
					enterASYNCNETSERVICE();
				}
				catch (std::exception& e) {
					if (output)
						*output << L"[CAMERA " + getTypeWString() + L"] Exception:";
					if (output)
						*output << e.what();
				}
				catch (char* c) {
					if (output)
						*output << L"[CAMERA " + getTypeWString() + L"] Exception:";
					if (output)
						*output << std::string(c);
				}
				if (terminateThreads)
					break;
				Sleep(1000);
			}
			setFrameStatus(false);
			setConnectStatus(false);
		}
	}
	DebugTerminal* output = NULL; 


	//Tracking
	ImageTracker imageTracker;
	bool selectingRegion = false;
	cv::Rect2d selectedRegion;
	bool sButtons = false;
	HWND sb_Label, sb_Cancel, sb_MF, sb_KCF, sb_YC, sb_Label2, sb_Save;
	HWND sb_Stop;
	void showSelectButtons() {
		sButtons = true;
		ShowWindow(sb_Label, SW_SHOW);
		ShowWindow(sb_Label2, SW_SHOW);
		ShowWindow(sb_Cancel, SW_SHOW);
		ShowWindow(sb_MF, SW_SHOW);
		ShowWindow(sb_KCF, SW_SHOW);
		ShowWindow(sb_YC, SW_SHOW);
		ShowWindow(sb_Save, SW_SHOW);
	}
	void sbCallback(int sb) {
		if (sb == 2)
			imageTracker.trackRegion(TRACKER_MF, selectedRegion, rawCamera);
		else if (sb == 3)
			imageTracker.trackRegion(TRACKER_KCF, selectedRegion, rawCamera);
		else if (sb == 4)//TEMP 
		{
		}
		else if (sb == 6)//TEMP 
		{
			cv::imwrite("data/images/savedimage.jpg", rawCamera(selectedRegion));
		}
		hideSelectButtons();
	}
	void hideSelectButtons() {
		sButtons = false;
		ShowWindow(sb_Label, SW_HIDE);
		ShowWindow(sb_Label2, SW_HIDE);
		ShowWindow(sb_Cancel, SW_HIDE);
		ShowWindow(sb_MF, SW_HIDE);
		ShowWindow(sb_KCF, SW_HIDE);
		ShowWindow(sb_YC, SW_HIDE);
		ShowWindow(sb_Save, SW_HIDE);
	}
	void mouseLeft(bool stat, int x, int y) {
		if (!init)
			return;
		if (!selectingRegion&&!stat)
			return;
		if (sButtons)
			return;
		if (!hasFrame()&&stat)
			return;
		selectingRegion = stat;
		if (selectingRegion) {
			selectedRegion.x = x;
			selectedRegion.y = y;
			selectedRegion.width = 0;
			selectedRegion.height = 0;
		}
		else {
			showSelectButtons();
		}
	}
	void mouseMove(int x, int y) {
		if (selectingRegion) {
			if (x<selectedRegion.x)
				selectedRegion.x = x;
			if (y<selectedRegion.y)
			selectedRegion.y = y;
			selectedRegion.width = x - selectedRegion.x;
			selectedRegion.height = y - selectedRegion.y;
		}
	}
	void procRaw() {

		framesProcessed++;
		if (framesProcessed>10) {
			actualFPS = (float)framesProcessed / ((std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - untrustworthy_secstart) / 1000.0);
			untrustworthy_secstart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			framesProcessed = 0;
		}

		//Perform tracking
		imageTracker.update(rawCamera);
		//Send AABB
		if (imageTracker.getIsTracking())
			parentcontrol->sendAABB(0);

		//Display (change the getMatToDisplay to change what gets displayed)
		if (renHBITMAP)
		{
			DeleteObject(renHBITMAP);
		}
		cv::Mat renMe = getMatToDisplay();
		Gdiplus::Bitmap bitmap(renMe.cols, renMe.rows, renMe.step1(), PixelFormat24bppRGB, renMe.data);
		bitmap.GetHBITMAP(Gdiplus::Color::Brown, &renHBITMAP);//Copies over to an object

		RCMC();

		InvalidateRect(thisWindow, NULL, FALSE);
	}
	void initSB() {
		sb_Label = CreateWindow(L"STATIC", L"Track using:", WS_CHILD, 30, 30, 100, 15, thisWindow, NULL, NULL, NULL);
		sb_MF = CreateWindow(L"BUTTON",     L"MF", WS_CHILD, 30, 30+ 30, 100, 30, thisWindow, (HMENU)2, NULL, NULL);
		sb_KCF = CreateWindow(L"BUTTON",    L"KCF", WS_CHILD, 30, 30 + 60, 100, 30, thisWindow, (HMENU)3, NULL, NULL);
		sb_YC = CreateWindow(L"BUTTON",     L"YC", WS_CHILD, 30, 30 + 90, 100, 30, thisWindow, (HMENU)4, NULL, NULL);
		RECT cr;
		GetClientRect(thisWindow, &cr);
		sb_Stop = CreateWindow(L"BUTTON", L"STOP TRACKING", WS_CHILD, cr.right/2 - 75, cr.bottom - 30, 150,30,thisWindow, (HMENU)5, NULL, NULL);
		sb_Label2 = CreateWindow(L"STATIC", L"Other:", WS_CHILD, 30, 120 + 30 + 30, 100, 15, thisWindow, NULL, NULL, NULL);
		sb_Save = CreateWindow(L"BUTTON", L"SAVE", WS_CHILD, 30, 120 + 30 + 30 + 30, 100, 30, thisWindow, (HMENU)6, NULL, NULL);
		sb_Cancel = CreateWindow(L"BUTTON", L"CANCEL", WS_CHILD, 30,  120 + 30 + 30 + 30 + 30, 100, 30, thisWindow, (HMENU)1, NULL, NULL);
	}
	void stopSB() {
		DestroyWindow(sb_Stop); sb_Stop = NULL;
		DestroyWindow(sb_Cancel);
		DestroyWindow(sb_MF);
		DestroyWindow(sb_KCF);
		DestroyWindow(sb_YC);
		DestroyWindow(sb_Label);
		DestroyWindow(sb_Label2);
		DestroyWindow(sb_Save);
	}
	int id = -1;
public:
	int getid() { return id; }
	Camera() :socket(io_service) {

	}
	void usbChange() {
		if (init)
			if (cameraType == CAMERA_LOCAL)
			{
				if (!connected)
					tryConnectUSB();
				else
					checkUSB();
			}
	}
	static void cameraPTStatic(Camera* _this) {
		_this->cameraPT();
	}
	HWND getWindow() {
		if (init)
		return thisWindow;
		return false;
	}
	void cease() {
		//Unlink all callbacks
		parentcontrol = NULL;
		imageTracker.cease();
	}
	void resize(int _wX, int _wY, int _wR, int _wB) {
		if (init)
		{
			windowLocation.left = _wX;
			windowLocation.top = _wY;
			windowLocation.right = _wR;
			windowLocation.bottom = _wB;
			specResize();
		}
	}
	ControlPanelCallback* parentcontrol;
	void initialize(int _id, ControlPanelCallback* _parentcontrol, HWND _parent, HINSTANCE _instance, DebugTerminal* _output, int _wX, int _wY, int _wR, int _wB, int _localCameraIndex, int _timeBetweenFrames) {
		if (init)
			return;
		id = _id;
		parentcontrol = _parentcontrol;
		parent = _parent;
		instance = _instance;
		output = _output;
		windowLocation.left = _wX;
		windowLocation.top = _wY;
		windowLocation.right = _wR;
		windowLocation.bottom = _wB;
		localCameraIndex = _localCameraIndex;
		cameraType = CAMERA_LOCAL;
		setConnectStatus(false);
		setFrameStatus(false);
		timeBetweenFrames = _timeBetweenFrames;

		makeWindow();


		imageTracker.initialize(output, _parentcontrol);

		initSB();

		init = true;


		terminateThreads = false;
		cameraProcessingThread = new std::thread(cameraPTStatic, this);//Only access the mats from this thread

	}
	void initialize(int _id, ControlPanelCallback* _parentcontrol, HWND _parent, HINSTANCE _instance, DebugTerminal* _output, int _wX, int _wY, int _wR, int _wB, std::string _ip, std::string _port, std::string _getRequest) {
		if (init)
			return;
		id = _id;
		parentcontrol = _parentcontrol;
		parent = _parent;
		instance = _instance;
		output = _output;
		windowLocation.left = _wX;
		windowLocation.top = _wY;
		windowLocation.right = _wR;
		windowLocation.bottom = _wB;
		ip = _ip;
		port = _port;
		getRequest = _getRequest;
		cameraType = CAMERA_NETWORK;
		setConnectStatus(false);
		setFrameStatus(false);

		makeWindow();

		imageTracker.initialize(output, _parentcontrol);


		initSB();

		init = true;

		terminateThreads = false;
		cameraProcessingThread = new std::thread(cameraPTStatic, this);//Only access the mats from this thread

	}
	void shutdown() {
		if (!init)
			return;
		
		parentcontrol = NULL;
		terminateThreads = true;
		cameraProcessingThread->join();
		delete cameraProcessingThread;
		cameraProcessingThread = NULL;
		terminateThreads = false;
		setConnectStatus(false);
		setFrameStatus(false);
		output = NULL;
		selectingRegion = false;

		imageTracker.shutdown();

		DestroyWindow(thisWindow);


		stopSB();

		if (renHBITMAP)
		{
			DeleteObject(renHBITMAP);
			renHBITMAP = NULL;
		}

		cameraType = CAMERA_NONE;

		init = false;
	}
	bool hasFrame() {
		if (init) {
			return frameReady;
		}
		return false;
	}
	bool isConnected() {
		if (init) {
			return connected;
		}
		return false;
	}
	cv::Rect2d getAABB() {
		if (!init)
			return cv::Rect2d(-1, -1, -1, -1);
		return imageTracker.getTrackRegion();
	}
	cv::Mat getMatToDisplay() {
		return rawCamera;
	}
	HBITMAP getHBITMAP() {
		return renHBITMAP;
	}
	std::wstring getTypeWString() {
		if (cameraType == CAMERA_LOCAL)
			return  L"LOCAL " + std::to_wstring(localCameraIndex);
		else if (cameraType == CAMERA_NETWORK) {
			std::wstring wip, wport;
			wip.assign(ip.begin(), ip.end());
			wport.assign(port.begin(), port.end());
			return  L"NETWORK " + wip + L":"+wport;
		}
		return L"NONE";
	}

	cv::Rect2d imageSearchHIST(std::string path) {
		if (!init || !frameReady)
			return cv::Rect2d(-1,-1,-1,-1);
		return ImageSearch::HIST(rawCamera, path);
	}
	bool hasTrackingOrders()
	{
		if (!init)
			return false;
		return imageTracker.getHasTrackingOrders();
	}
	bool isTracking() {
		if (!init)
			return false;
		return imageTracker.getIsTracking();
	}
	void resetTracker() {
		if (!init)
			return;
		imageTracker.reset();
	}
	string colorMapURL = "data/colormap.txt";
	bool trackImage(int imgid, int method, int trackmethod, vector<int>* keys, vector<string>* values)  {
		if (!init)
			return false;
		if (!isConnected() || !hasFrame())
			return false;
		cv::Rect2d t(-1, -1, -1, -1);
		if (method == 0) {
			for (int i = 0; i < keys->size(); i++) {
				if ((*keys)[i] == imgid) {
					t = ImageSearch::HIST(rawCamera, (*values)[i]); 
					break;
				}
			}
		}
		else if (method == 1) {
			int ilh, ihh, ils, ihs, ilv, ihv;
			DataDecodeSingleArgStringKey colormap;
			colormap.decodeFile(colorMapURL);//Add advanced selection later
			if (colormap.keys.size() <= 0)
			{
				if (output)
					*output << "Image search failed. Invalid colormap.txt.";
				return false;
			}
			ilh = stoi(colormap.lookup("ILH"));
			ihh = stoi(colormap.lookup("IHH"));
			ils = stoi(colormap.lookup("ILS"));
			ihs = stoi(colormap.lookup("IHS"));
			ilv = stoi(colormap.lookup("ILV"));
			ihv = stoi(colormap.lookup("IHV"));
			t = ImageSearch::COLORMAP(rawCamera, ilh, ihh, ils, ihs, ilv, ihv);
			if (t.area() <= stoi(colormap.lookup("MINSIZE")))
			{
				if (output)
					*output << "Image search failed. MINSIZE not met.";
				return false;
			}
		}
		else {

		}
		if (t.x == -1)
		{
			if (output)
				*output << "Image search failed. t.x==-1";
			return false;
		}
		imageTracker.trackRegion(TRACKER_MF, t, rawCamera);
		return true;
	}
	bool track(int trackmethod, int x, int y, int w, int h)  {
		if (!init)
			return false;
		if (!isConnected() || !hasFrame())
			return false;
		ImageTrackerType method = ImageTrackerType::TRACKER_MF;
		if (trackmethod == 0)
			method = ImageTrackerType::TRACKER_MF;
		else if (trackmethod == 1)
			method = ImageTrackerType::TRACKER_KCF;


		imageTracker.trackRegion(method, cv::Rect2d(x,y,w,h), rawCamera);

		return true;
	}
	void outConnectionInfo() {
		if (!output)
			return;
		if (parentcontrol)
			parentcontrol->showDebugConsole();
		if (!init) {
			*output << "Camera not initialized.";
			return;
		}
		*output << "Camera connection info for camera ID " + to_string(id) + ":";
		if (cameraType == CAMERA_LOCAL) {
			*output << "Camera Type: LOCAL";
			*output << "Local Index: " + std::to_string(localCameraIndex);
		}
		else if (cameraType == CAMERA_NETWORK) {
			*output << "Camera Type: NETWORK";
			*output << "IP: " + ip;
			*output << "Port: " + port;
		}
		else {
			*output << "Camera Type: NONE";
		}
	}
	friend LRESULT CALLBACK Camera_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

LRESULT CALLBACK Camera_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Camera* curCamera = (Camera*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	

	PAINTSTRUCT ps;
	HDC hdc;
	switch (message)
	{
	case WM_SIZE:
		if (curCamera)
			if (curCamera->sb_Stop)
				SetWindowPos(curCamera->sb_Stop, NULL, LOWORD(lParam) / 2 - 75, HIWORD(lParam) - 30, 0, 0, SWP_NOSIZE);
		break;
	case WM_COMMAND:
		if (curCamera)
			switch (wParam) {
			case 1:
				curCamera->sbCallback(1);
				break;
			case 2:
				curCamera->sbCallback(2);
				break;
			case 3:
				curCamera->sbCallback(3);
				break;
			case 4:
				curCamera->sbCallback(4);
				break;
			case 5:
				curCamera->imageTracker.reset();
				break;
			case 6:
				curCamera->sbCallback(6);
				break;
			}
		break;
	case WM_PAINT: {
		hdc = BeginPaint(hWnd, &ps);

		HBRUSH tra = CreateSolidBrush(RGB(0, 0, 0));

		RECT win;
		GetClientRect(hWnd, &win);

		if (curCamera)
		{
			

			if (!curCamera->isConnected() || !curCamera->hasFrame())
			{
				FillRect(hdc, &win, tra);

				SetBkMode(hdc, TRANSPARENT);

				std::wstring str;
				if (!curCamera->isConnected())
					str = L"No Connection To Camera";
				else if (!curCamera->hasFrame())
					str = L"Connected, No Frame To Display";

				std::wstring tstr = curCamera->getTypeWString();

				SetTextAlign(hdc, TA_CENTER);
				SetTextColor(hdc, RGB(255, 255, 255));
				HFONT hFont = CreateFont(30, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, L"SYSTEM_FIXED_FONT");
				HFONT hTmp = (HFONT)SelectObject(hdc, hFont);
				TextOut(hdc, (curCamera->windowLocation.right - curCamera->windowLocation.left) / 2, (curCamera->windowLocation.bottom - curCamera->windowLocation.top) / 2, str.c_str(), str.size());
				DeleteObject(SelectObject(hdc, hTmp)); 
				
				hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, TRUE, 0, 0, 0, 0, 0, 2, 0, L"SYSTEM_FIXED_FONT");
				hTmp = (HFONT)SelectObject(hdc, hFont);
				if (curCamera->cameraType == CAMERA_NETWORK) {
					TextOut(hdc, (curCamera->windowLocation.right - curCamera->windowLocation.left) / 2, (curCamera->windowLocation.bottom - curCamera->windowLocation.top) - 20 - 20, tstr.c_str(), tstr.size());
					wstring dws = L"CHECK FIREWALL. RIGHT-CLICK FOR CONNECTION INFO";
					TextOut(hdc, (curCamera->windowLocation.right - curCamera->windowLocation.left) / 2, (curCamera->windowLocation.bottom - curCamera->windowLocation.top) - 20, dws.c_str(), dws.size());
				}
				else {
					TextOut(hdc, (curCamera->windowLocation.right - curCamera->windowLocation.left) / 2, (curCamera->windowLocation.bottom - curCamera->windowLocation.top) - 20 - 20, tstr.c_str(), tstr.size());
					wstring dws = L"RIGHT-CLICK FOR CONNECTION INFO";
					TextOut(hdc, (curCamera->windowLocation.right - curCamera->windowLocation.left) / 2, (curCamera->windowLocation.bottom - curCamera->windowLocation.top) - 20, dws.c_str(), dws.size());
				}
				DeleteObject(SelectObject(hdc, hTmp));

				SetBkMode(hdc, OPAQUE);
			}
			else {//Frame
				//FillRect(hdc, &win, tra);

				if (curCamera)//REDO THIS CODE, WHY COPY FROM BITMAP TO HBITMAP??? Waste of processing? @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
				{
					HBITMAP renMe = curCamera->getHBITMAP();
					if (renMe)
					{
						HDC hdcMem = CreateCompatibleDC(hdc);
						HGDIOBJ oldBitmap = SelectObject(hdcMem, renMe);
						BITMAP bitmap;
						GetObject(renMe, sizeof(bitmap), &bitmap);
						BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

						SelectObject(hdcMem, oldBitmap);
						DeleteDC(hdcMem);

						std::wstring str = std::to_wstring(curCamera->actualFPS) + L"FPS " +std::to_wstring(bitmap.bmWidth) + L"x" + std::to_wstring(bitmap.bmHeight);

						SetTextAlign(hdc, TA_LEFT);
						SetTextColor(hdc, RGB(0, 0, 0));
						HFONT hFont = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, L"SYSTEM_FIXED_FONT");
						HFONT hTmp = (HFONT)SelectObject(hdc, hFont);
						TextOut(hdc, (win.right - win.left) / 2 - bitmap.bmWidth / 2 + 1, (win.bottom - win.top) / 2 + bitmap.bmHeight / 2 - 20, str.c_str(), str.size());
						std::wstring tstr = curCamera->getTypeWString();
						SetTextAlign(hdc, TA_RIGHT);
						TextOut(hdc, (win.right - win.left) / 2 + bitmap.bmWidth / 2, (win.bottom - win.top) / 2 + bitmap.bmHeight / 2 - 20, tstr.c_str(), tstr.size());
						DeleteObject(SelectObject(hdc, hTmp));
					}
				}


			}

			if (curCamera) {
				if (curCamera->imageTracker.getHasTrackingOrders())
				{
					cv::Rect2d tRegion = curCamera->imageTracker.getTrackRegion();
					HPEN hPen;
					if (curCamera->imageTracker.getIsTracking()) {
						hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
						SetBkColor(hdc, RGB(255, 255, 255));
					}
					else {
						hPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
						SetBkColor(hdc, RGB(255, 0, 0));
					}
					HGDIOBJ hOldPen = SelectObject(hdc, hPen);
					SetTextAlign(hdc, TA_LEFT);
					std::wstring trackString = L"Track (" + curCamera->imageTracker.getWStringAbbreviatedType() + L")";
					TextOut(hdc, tRegion.x, tRegion.y, trackString.c_str(), trackString.length());
					MoveToEx(hdc, tRegion.x, tRegion.y, NULL);
					LineTo(hdc, tRegion.x + tRegion.width, tRegion.y);
					LineTo(hdc, tRegion.x + tRegion.width, tRegion.y + tRegion.height);
					LineTo(hdc, tRegion.x, tRegion.y + tRegion.height);
					LineTo(hdc, tRegion.x, tRegion.y);
					SelectObject(hdc, hOldPen);
					DeleteObject(hPen);
					SetBkColor(hdc, RGB(255, 255, 255));
				}

				if (curCamera->selectingRegion || curCamera->sButtons) {
					HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
					HGDIOBJ hOldPen = SelectObject(hdc, hPen);
					SetTextAlign(hdc, TA_LEFT);
					TextOut(hdc, curCamera->selectedRegion.x, curCamera->selectedRegion.y, L"Select", 6);
					MoveToEx(hdc, curCamera->selectedRegion.x, curCamera->selectedRegion.y, NULL);
					LineTo(hdc, curCamera->selectedRegion.x + curCamera->selectedRegion.width, curCamera->selectedRegion.y);
					LineTo(hdc, curCamera->selectedRegion.x + curCamera->selectedRegion.width, curCamera->selectedRegion.y + curCamera->selectedRegion.height);
					LineTo(hdc, curCamera->selectedRegion.x, curCamera->selectedRegion.y + curCamera->selectedRegion.height);
					LineTo(hdc, curCamera->selectedRegion.x, curCamera->selectedRegion.y);
					SelectObject(hdc, hOldPen);
					DeleteObject(hPen);
				}
			}
			if (curCamera->colorMapCalibration)
			{
				std::wstring str = L"Middle-Click To Save Color Map.";
				SetTextAlign(hdc, TA_CENTER);
				SetTextColor(hdc, RGB(0, 0, 0));
				HFONT hFont = CreateFont(30, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, L"SYSTEM_FIXED_FONT");
				HFONT hTmp = (HFONT)SelectObject(hdc, hFont);
				TextOut(hdc, (win.right - win.left) / 2, 0, str.c_str(), str.size());
				DeleteObject(SelectObject(hdc, hTmp));
			}
		}

		DeleteObject(tra);

		EndPaint(hWnd, &ps); }
				   break;
	case WM_LBUTTONDOWN:
		if (curCamera)
			curCamera->mouseLeft(true, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	case WM_LBUTTONUP:
		if (curCamera)
			curCamera->mouseLeft(false, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	case WM_MOUSEMOVE:
		if (curCamera)
			curCamera->mouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	case WM_RBUTTONUP: {
		if (curCamera)
			curCamera->outConnectionInfo();
	}break;
	case WM_MBUTTONUP: {
		if (curCamera)
			curCamera->MUP();
	}break;
	case WM_MBUTTONDOWN: {
	}break;
	case WM_DESTROY:

		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}