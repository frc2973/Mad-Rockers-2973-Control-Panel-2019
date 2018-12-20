#pragma once

#include <opencv2\video.hpp>
#include <opencv2\videoio.hpp>
#include <opencv2\opencv.hpp>

#include <opencv2/tracking.hpp>
#include <opencv2/tracking/tracker.hpp>

#include "opencv2/imgproc/imgproc.hpp"

namespace ImageSearch {
	using namespace cv;
	using namespace std;
	Rect2d HIST(Mat raw, string path) {
		Mat ret = imread(path);
		if (ret.data == NULL)
			return Rect2d(-1, -1, -1, -1);
		Mat result(raw.rows - ret.rows + 1, raw.cols - ret.cols + 1, CV_32FC1);
		matchTemplate(raw, ret, result, 1);
		normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());
		double minVal, maxVal;
		Point minLoc, maxLoc;
		Point matchLoc;
		minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());
		matchLoc = minLoc;
		return Rect2d(matchLoc,matchLoc+Point(ret.cols,ret.rows));
	}
	Rect COLORMAP(Mat mat, int iLowH, int iHighH, int iLowS, int iHighS, int iLowV, int iHighV) {
		using namespace cv;
		using namespace std;
		Mat imgHSV;
		cvtColor(mat, imgHSV, COLOR_BGR2HSV);
		Mat imgThresholded;
		inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded);
		erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		findContours(imgThresholded, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
		vector<vector<Point> > contours_poly(contours.size());
		vector<Rect> boundRect(contours.size());
		for (int i = 0; i < contours.size(); i++)
		{
			approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
			boundRect[i] = boundingRect(Mat(contours_poly[i]));
		}
		Mat drawing = Mat::zeros(imgThresholded.size(), CV_8UC3);
		int indexOfMax = 0;
		int largestArea = 0;
		for (int i = 0; i < contours.size(); i++) {
			if (boundRect[i].area() > largestArea) {
				indexOfMax = i;
				largestArea = boundRect[i].area();
			}
		}
		if (contours.size() <= 0)
			return Rect(-1, -1, -1, -1);
		return boundRect[indexOfMax];
	}
}