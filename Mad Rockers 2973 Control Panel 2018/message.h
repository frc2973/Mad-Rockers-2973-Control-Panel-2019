#ifndef HEADERS_NETWORKING_MESSAGE_H_
#define HEADERS_NETWORKING_MESSAGE_H_

/*
Mad Rockers Control 2018
Developed by FIRST Team 2973 software team member Evan Krohn for 2018 competition year
Networking protocol version (Underlying communications):
*/
#define NETWORKING_PROTOCOL_VERSION 23
/*
Version notes:
Appears to be bug-free. Fixed the issues relating with memory crashes on an ARM processor.
Networking version:
*/
#define NETWORKING_VERSION 23004
/*
Version notes:
Appears to be bug-free. 
*/

#include <cstdlib>
#include <queue>
#include <iostream>
#include <thread>

class MRCCommand {
public:
	void* compData = NULL;
	enum Commands { //NET23 commands
		MRCC_WELCOME,//implemented T2 checks network versions
		MRCC_ROBOT_STATUS,//implemented T2 [0=unspecified 1=debug 2=waiting 3=auto 4=tele 5=postgame -1=get] ignored if sent to server
		MRCC_CAMERASTATUS,//implemented T4 [camera code, 0 = not connected 1 = connected, 0 = no frame 1 = has frame , not used] if sent to server server fills out and returns 
		MRCC_TRACKINGSTATUS,//implemented T4 [camera code, 0=no orders 1=has tracking orders, 0 = not tracking 1= tracking, not used] ignored if sent to server 
		MRCC_STOPTRACKING,//implemented T2 [camera code]
		MRCC_TRACKINGAABB,//implemented T3 [camera code, not used, x, y, w, h] server ignores this message. wait for it to be sent automatically. 
		MRCC_TRACKIMAGE,// implemented T4 [camera code, image id, 0=HIST 1=COLORMAP, 0=MF 1=KCF]
		MRCC_TRACK,// implemented T3 [camera code, 0=MF 1=KCF, x, y, w, h]
		MRCC_SETAUTOPILOT, // implemented T2 [0=not autopilot 1=autopilot] client ignores this message. Server will set autopilot to true server-side on autonomous, no need to send this message then. During robot status changes, the server will automatically disable autopilot status server-side. This should be sent when using macros on teleop.
		MRCC_TRACKRETURN,//implemented t4 [camid, 0=failed 1=success, not used, not used]
		MRCC_BATTERY,//implemented t3 [float voltage, not used right now but maybe for clock?]
		MRCC_AUTODATA,//implemented t4 [0=starting postition 1=auto plan, value, value, value]
		MRCC_SETAUTODATA,//implemented T2 [0=no auto data, 1=autodata]
		MRCC_SOUND //implemented T2 [0=omaweu]
		//MRCC_DEBUG //implemented T5 Print message to console
		//MRCC_STOREWAYPOINTS
		
	};
	class MRCCommandBodyBase {
	public:
		unsigned char type;
		union ADATA {
			struct T1TYPE {
				float f1;
				float f2;
			} T1;
			struct T2TYPE {
				int i1;
			} T2;
			struct T3TYPE {
				int i1; int i2;
				float f1;
				float f2;
				float f3;
				float f4;
			} T3;
			struct T4TYPE {
				int i1, i2, i3, i4;
			} T4;
		} aData;
		int bodySizeInBytes() {
			if (type == 1)
				return sizeof(aData.T1);
			else if (type == 2)
				return sizeof(aData.T2);
			else if (type == 3)
				return sizeof(aData.T3);
			else if (type == 4)
				return sizeof(aData.T4);
			else {
				throw("NTYPE");
				return 0;
			}
		}
		void* getBody() {
			return &aData;
		}
		void copyData(void* data) {
			aData = *reinterpret_cast<ADATA*>(data);
		}
		unsigned char getType() {
			return type;
		}
	};
	struct MRCCommandHeader {
		unsigned char verificationCode = 23;
		unsigned char from;
		unsigned char to;
		unsigned char type;
		int command;
		int bodySizeInBytes;
	} mrch;
	bool decode_header() {
		if (mrch.verificationCode != 23)
			return false;
		return true;
	}
	MRCCommandBodyBase mrcb;
	void prepHeader() {
		mrch.type = mrcb.getType();
		mrch.bodySizeInBytes = mrcb.bodySizeInBytes();
	}
	void* data() {
		if (compData)
			free(compData);
		compData = malloc(totalDataSize());
		memcpy(compData, &mrch, sizeof(MRCCommandHeader));
		memcpy(
			reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(compData)
				+ sizeof(MRCCommandHeader)), mrcb.getBody(),
			mrch.bodySizeInBytes);
		return compData;
	}
	int totalDataSize() {
		return mrch.bodySizeInBytes + sizeof(MRCCommandHeader);
	}
	void setType1(Commands command, float f1, float f2) {
		mrch.type = (unsigned char)1;
		mrch.from = 0;
		mrch.to = 1;
		mrch.command = command;
		mrcb.type = (unsigned char)1;
		mrcb.aData.T1.f1 = f1;
		mrcb.aData.T1.f2 = f2;
		prepHeader();
	}
	void setType2(Commands command, int i1) {
		mrch.type = (unsigned char)2;
		mrch.from = 0;
		mrch.to = 1;
		mrch.command = command;
		mrcb.type = (unsigned char)2;
		mrcb.aData.T2.i1 = i1;
		prepHeader();
	}
	void setType3(Commands command, int i1, int i2, int f1, int f2, int f3, int f4) {
		mrch.type = (unsigned char)3;
		mrch.from = 0;
		mrch.to = 1;
		mrch.command = command;
		mrcb.type = (unsigned char)3;
		mrcb.aData.T3.i1 = i1;
		mrcb.aData.T3.i2 = i2;
		mrcb.aData.T3.f1 = f1;
		mrcb.aData.T3.f2 = f2;
		mrcb.aData.T3.f3 = f3;
		mrcb.aData.T3.f4 = f4;
		prepHeader();
	}
	void setType4(Commands command, int i1, int i2, int i3, int i4) {
		mrch.type = (unsigned char)4;
		mrch.from = 0;
		mrch.to = 1;
		mrch.command = command;
		mrcb.type = (unsigned char)4;
		mrcb.aData.T4.i1 = i1;
		mrcb.aData.T4.i2 = i2;
		mrcb.aData.T4.i3 = i3;
		mrcb.aData.T4.i4 = i4;
		prepHeader();
	}
	MRCCommand() {

	}
	~MRCCommand() {
		if (compData) {
			free(compData);
			compData = NULL;
		}
	}
};

#endif /* HEADERS_NETWORKING_MESSAGE_H_ */
