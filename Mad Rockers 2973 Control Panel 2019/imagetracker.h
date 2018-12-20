#pragma once

#include <opencv2\video.hpp>
#include <opencv2\videoio.hpp>
#include <opencv2\opencv.hpp>

#include <opencv2/tracking.hpp>
#include <opencv2/tracking/tracker.hpp>

#include "debugterminal.h"

enum ImageTrackerType {
	TRACKER_NONE, TRACKER_KCF, TRACKER_MF, TRACKER_CUSTOM_YELLOWBLOCK
};

class ImageTracker {
	ImageTrackerType trackerType = TRACKER_NONE;
	bool init = false;
	bool hasTrackingOrders = false;
	bool isTrackingObject = false;
	cv::Ptr<cv::Tracker> tracker;
	cv::Rect2d trackObjPos;
	void setIsTrackingObject(bool val) {
		if (val == isTrackingObject)
			return;
		isTrackingObject = val;
		if (parentcontrol)
			parentcontrol->sendTrackingStatus();
	}
	void setHasTrackingOrders(bool val) {
		if (val == hasTrackingOrders)
			return;
		hasTrackingOrders = val;
		if(parentcontrol)
		parentcontrol->sendTrackingStatus();
	}
	//To prevent an update or reset while the other occurs
	bool resettingTracker = false;
	bool updating = false;
	DebugTerminal* output;
	ControlPanelCallback* parentcontrol;
public:
	void cease() {
		parentcontrol = NULL;
	}
	void reset() {
		if (!init)
			return;
		if (!hasTrackingOrders)
			return;

		//To prevent an update or reset while the other occurs
		resettingTracker = true;
		int numMSecs = 0;
		while (updating)
		{
			numMSecs++;
			Sleep(1);
		}
		if (output&&numMSecs>0)
			*output << "Image tracker slept " + std::to_string(numMSecs) + "ms to prevent trying to clear in-use memory.";

		setIsTrackingObject(false);
		setHasTrackingOrders(false);
		if (trackerType != TRACKER_NONE)
		{
			tracker->clear();
		}
		trackerType = TRACKER_NONE;

		//To prevent an update or reset while the other occurs
		resettingTracker = false;
	}
	void update(cv::Mat rawCamera) {
		if (!init || !hasTrackingOrders)
			return;

		//To prevent an update or reset while the other occurs
		if (resettingTracker) {
			if (output)
				*output << "Image tracker skipped an update to prevent trying to use cleared memory.";
			return;
		}
		updating = true;

		bool rval = tracker->update(rawCamera, trackObjPos);
		if (rval != isTrackingObject)
		{
			setIsTrackingObject(rval);
		}
		else
		{

		}

		//To prevent an update or reset while the other occurs
		updating = false;
	}
	cv::Rect2d getTrackRegion() {
		return trackObjPos;
	}
	std::wstring getWStringAbbreviatedType()
	{
		if (trackerType == TRACKER_MF)
			return L"MF";
		else if (trackerType == TRACKER_KCF)
			return L"KCF";
		return L"ERR";
	}
	void trackRegion(ImageTrackerType _trackerType, cv::Rect2d trackRegion, cv::Mat rawCamera) {
		if (!init)
			return;
		if (hasTrackingOrders)
			reset();
		if (_trackerType == TRACKER_KCF)
			tracker = cv::TrackerKCF::create();
		else if (_trackerType == TRACKER_MF)
			tracker = cv::TrackerMedianFlow::create();
		if (!tracker->init(rawCamera, trackRegion))
			throw("tracker init failed");
		setHasTrackingOrders(true);
		setIsTrackingObject(true);
		trackerType = _trackerType;
		trackObjPos = trackRegion;
	}
	bool getIsTracking() {
		if (!init)
			return false;
		return isTrackingObject;
	}
	bool getHasTrackingOrders() {
		if (!init)
			return false;
		return hasTrackingOrders;
	}
	void initialize(DebugTerminal* _output, ControlPanelCallback* _parentcontrol) {
		if (init)
			return;
		output = _output;
		parentcontrol = _parentcontrol;
		trackerType = TRACKER_NONE;
		init = true;
	}
	void shutdown() {
		if (!init)
			return;
		reset();
		trackerType = TRACKER_NONE;
		output = NULL;
		init = false;
	}
	~ImageTracker() {
		shutdown();
	}
};