#pragma once

class ControlPanelCallback {
public :
	virtual void serverStatusUpdated() = 0;
	virtual void sendCamerasStatus() = 0;
	virtual void sendTrackingStatus() = 0;
	virtual void sendAABB(int camid) = 0;
	virtual void stopTracking(int camid) = 0;
	virtual void trackImage(int camid, int imgid, int method, int trackmethod) = 0;
	virtual void track(int camid, int trackmethod, int x, int y, int w, int h) = 0;
	virtual void setAutoPilot(bool status) = 0;
	virtual void showDebugConsole() = 0;
	virtual void setBattery(float volt) = 0;
	virtual void sendAutoData(int i1, int i2, int i3, int i4) = 0;
	virtual void setAutoData(int status) = 0;
};