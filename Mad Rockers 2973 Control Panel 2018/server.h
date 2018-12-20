#pragma once

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <thread>
#include <vector>
#include <ctime>
#include <chrono>

#include <Windows.h>
#include <MMSystem.h>

#include "callbacks.h"
#include "message.h"

using boost::asio::ip::tcp;

//Restructure this is it looks ugly with allt he crazy networking

enum RobotStatus {
	RS_NOTCONNECTED,
	RS_UNKNOWN,
	RS_DISABLED,
	RS_AUTONOMOUS,
	RS_TELEOP
};

class Server {
	bool init = false;
	bool threadsShouldTerminate = false;
	bool autoPilotStatus = false;
	bool setAutoPilotStatus(bool status) {
		if (autoPilotStatus == status)
			return false;
		autoPilotStatus = status;
		if (output)
			*output << std::string("Auto pilot status updated: ") + (status ? "ENABLED" : "DISABLED");
		if (parentcb) {
			parentcb->setAutoPilot(status);
		}
		return true;
	}
	RobotStatus robotStatus = RS_NOTCONNECTED;
	std::thread* thr;
	tcp::socket* curSocket = NULL;

	void setRobotStatus(RobotStatus rs) {
		if (!init)
			return;
		if (rs == robotStatus)
			return;

		if (robotStatus == RobotStatus::RS_NOTCONNECTED) {
			//PlaySound(TEXT("sounds/xp.wav"), NULL, SND_ASYNC | SND_FILENAME);
		}
		else if (rs == RobotStatus::RS_NOTCONNECTED) {
			//PlaySound(TEXT("sounds/rds.wav"), NULL, SND_ASYNC | SND_FILENAME);
		}

		if (rs == RS_TELEOP)
		{
			//PlaySound(TEXT("sounds/mii.wav"), NULL, SND_ASYNC | SND_FILENAME);
		}
		else if (robotStatus != RS_UNKNOWN && rs == RS_DISABLED)
		{
			//PlaySound(TEXT("sounds/rds.wav"), NULL, SND_ASYNC | SND_FILENAME);
		}

		robotStatus = rs;

		if (output)
			*output <<L"Server status updated: " + getStatusWString();
		parentcb->serverStatusUpdated();
		if (rs == RS_AUTONOMOUS)
			setAutoPilotStatus(true);
		else
			setAutoPilotStatus(false);
	}
	void handleMessage(MRCCommand& com) {
		output->setNetInfo(1, 0);

		switch (com.mrch.command) {
		case MRCCommand::MRCC_SOUND: {
			if (com.mrcb.aData.T2.i1 == 1) {
				//PlaySound(TEXT("sounds/omaw.wav"), NULL, SND_ASYNC | SND_FILENAME);
			}
		}break;
		case MRCCommand::MRCC_WELCOME:
		if (com.mrch.verificationCode!=23)
		{
			if (output)
				*output << L"The robot that connected had the wrong verification code. Should have been: 23, got: "+std::to_wstring(com.mrch.verificationCode);
			if (curSocket)
				curSocket->close();
		}
			break;
		case MRCCommand::MRCC_ROBOT_STATUS:
		{
			int rstoken = com.mrcb.aData.T2.i1;
			if (rstoken == 3)
				setRobotStatus(RS_AUTONOMOUS);
			else if (rstoken == 4)
				setRobotStatus(RS_TELEOP);
			else if (rstoken == 2)
				setRobotStatus(RS_DISABLED);
			else {
				if (output)
					*output << L"Server couldn't interpret a status: " + std::to_wstring(rstoken);
				setRobotStatus(RS_UNKNOWN);
			}
		}
			break;
		case MRCCommand::MRCC_CAMERASTATUS:
		{
			//Send status of all cameras
			if (parentcb)
				parentcb->sendCamerasStatus();
		}break;
		case MRCCommand::MRCC_TRACKINGSTATUS:
		{
			//Send status of all trackers
			if (parentcb)
				parentcb->sendTrackingStatus();
		}break;
		case MRCCommand::MRCC_STOPTRACKING:
		{
			if (parentcb)
				parentcb->stopTracking(com.mrcb.aData.T2.i1);
		}break;
		case MRCCommand::MRCC_TRACKIMAGE: {
			if (parentcb) {
			//MRCC_TRACKIMAGE,// T4 [camera code, image id, 0=HIST, 0=MF 1=KCF]
				parentcb->trackImage(com.mrcb.aData.T4.i1, com.mrcb.aData.T4.i2, com.mrcb.aData.T4.i3, com.mrcb.aData.T4.i4);
			}
		}break;
		case MRCCommand::MRCC_TRACK: {
			if (parentcb) {
				//MRCC_TRACK// T3 [camera code, 0=MF 1=KCF, x, y, w, h]
				parentcb->track(com.mrcb.aData.T3.i1, com.mrcb.aData.T3.i2, com.mrcb.aData.T3.f1, com.mrcb.aData.T3.f2, com.mrcb.aData.T3.f3, com.mrcb.aData.T3.f4);
			}
		}break;
		case MRCCommand::MRCC_SETAUTOPILOT: {
			setAutoPilotStatus(com.mrcb.aData.T2.i1);

		}break;
		case MRCCommand::MRCC_BATTERY: {
			if (parentcb)
				parentcb->setBattery(com.mrcb.aData.T3.f1);
		}break;
		case MRCCommand::MRCC_SETAUTODATA: {
			if (parentcb)
				parentcb->setAutoData(com.mrcb.aData.T2.i1);
		}break;
		}
	}
	class CONNECTION {
	public:
		tcp::acceptor acceptor;
		tcp::socket socket;
		Server* parent;
		MRCCommand incomingCommand;
		friend Server;
		void connection_died() {
			parent->setRobotStatus(RS_NOTCONNECTED);
			if (parent->output)
				*parent->output << "Server lost robot.";
			parent->curSocket = NULL;
			if (!parent->threadsShouldTerminate) {
				socket = tcp::socket(acceptor.get_io_service());
				acceptor.async_accept(socket, boost::bind(&CONNECTION::handle_accept, this));
			}
		}
		void r_header() {
			boost::asio::async_read(socket,
				boost::asio::buffer(&(incomingCommand.mrch),
					sizeof(MRCCommand::MRCCommandHeader)),
				[this](boost::system::error_code ec, std::size_t len)
			{
				if (!ec && incomingCommand.decode_header())
				{
					this->r_body();
				}
				else {
					socket.close();
					if (ec)
					{
						if (this->parent->output)
							*this->parent->output << "Server error during header read: "+ ec.message();
					}
					this->connection_died();
				}
			});
		}
		void r_body() {
			boost::asio::async_read(socket,
				boost::asio::buffer(incomingCommand.mrcb.getBody(), incomingCommand.mrch.bodySizeInBytes),
				[this](boost::system::error_code ec, std::size_t)
			{
				if (!ec)
				{
					this->parent->handleMessage(incomingCommand);
					this->r_header();
				}
				else {
					socket.close();
					if (ec)
					{
						if (this->parent->output)
							*this->parent->output << "Server error during body read: " + ec.message();
					}
					this->connection_died();
				}
			});
		}
		void handle_accept() {
			parent->curSocket = &socket;
			if (parent->output)
				*parent->output << "Server accepted robot.";
			parent->setRobotStatus(RS_UNKNOWN);
			//Send welcome message
			MRCCommand msg;
			msg.setType2(MRCCommand::Commands::MRCC_WELCOME, 1);
			parent->deliver(msg);
			//Request the status.
			msg.setType2(MRCCommand::Commands::MRCC_ROBOT_STATUS, -1);
			parent->deliver(msg);
			//Headers now
			r_header();
		}
		CONNECTION(boost::asio::io_service* _io_service,Server* _parent, int port) :parent(_parent), socket(*_io_service), acceptor(*_io_service, tcp::endpoint(tcp::v4(),port)) {
			acceptor.async_accept(socket, boost::bind(&CONNECTION::handle_accept, this));
		}
	};
	boost::asio::io_service* io_serviceg = NULL;
	void thr_server() {
		try {
			boost::asio::io_service io_service;
			io_serviceg = &io_service;
			CONNECTION con(&io_service, this, stoi(port));
			if (output)
				*output << "Running server IO service.";
			io_service.run();
		}
		catch (std::exception& e) {
			if (output)
				*output << "Server error: "<<e.what();
		}
		io_serviceg = NULL;
		if (output)
			*output << "Server ended.";
	}
	DebugTerminal* output;
	std::string port;
public:
	bool getAutoPilotStatus() {
		if (!init)
			return false;
		return autoPilotStatus;
	}
	void deliver(MRCCommand& msg) {
		if (!init||robotStatus == RS_NOTCONNECTED)
			return;

		try {
			if (curSocket)
			boost::asio::write(*curSocket,
				boost::asio::buffer(msg.data(), msg.totalDataSize()));
			output->setNetInfo(0, 1);
		}
		catch (std::exception& e) {
			if (output)
				*output << "Server error (while delivering): " << e.what();
		}

	}
	static void thr_static(Server* _this) {//calls member function (for threads)
		_this->thr_server();
	}
	Server() {

	}
	void cease() {
		//Unlink all callbacks
	}
	RobotStatus getStatus() {
		if (!init)
			return RS_NOTCONNECTED;
		return robotStatus;
	}
	std::wstring getStatusWString() {
		if (!init)
			return L"Server Not Initialized";
		if (robotStatus == RS_NOTCONNECTED)
			return L"Robot Not Connected";
		else if (robotStatus == RS_DISABLED)
			return L"Robot Connected (Disabled)";
		else if (robotStatus == RS_TELEOP)
			return L"Robot Connected (Teleop)";
		else if (robotStatus == RS_AUTONOMOUS)
			return L"Robot Connected (Autonomous)";
		else if (robotStatus == RS_UNKNOWN)
			return L"Robot Connected (Unknown)";
		else
			return L"Error";
	}
	ControlPanelCallback* parentcb;
	void initialize(DebugTerminal* _output, ControlPanelCallback* _parent, std::string _port) {
		if (init)
			return;
		output = _output;
		parentcb = _parent;
		port = _port;
		robotStatus = RS_NOTCONNECTED;
		threadsShouldTerminate = false;
		thr = new std::thread(thr_static, this);
		init = true; 
		robotStatus = RS_NOTCONNECTED;
	}
	void shutdown() {
		if (!init)
			return;
		threadsShouldTerminate = true;
		if (io_serviceg)
			io_serviceg->stop();
		thr->join();
		threadsShouldTerminate = false;
		delete(thr);
		parentcb = NULL;
		init = false;
	}
	bool isConnectedToRobot() {
		if (init) {
			return robotStatus!=RS_NOTCONNECTED;
		}
		return false;
	}
};