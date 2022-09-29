// TestQualisys7.cpp : Defines the entry point for the application.
//

#include <iostream>
#include <thread>

#include "Synch.h"
#include "QualisysConnection.h"
#include "AModeUSConnection.h"

// dependencies
#include <tclap/CmdLine.h>


int main()
{
	std::cout << "Qualisys + Ultrasound TCP Connection." << std::endl;

	bool useQualisys = true;
	bool useAMode    = true;
	bool useBMode	 = false;

	// initalize value for Qualisys system
	std::string qualisys_ip = "127.0.0.1";
	unsigned short qualisys_port = 22222;
	std::string qualisys_password = "password";
	std::string qualisys_outputdir = "D:/qualisyslog";

	// initialize value for A-mode ultrasound system
	std::string amode_ip = "192.168.1.2";
	std::string amode_port = "6340";
	std::string amode_outputdir = "D:\\amodestream\\log";
	int amode_mode = 0;
	int amode_samples = 1500;
	int amode_probes = 30;

	if (useQualisys)
	{
		// Qualisys is the center of the attention here, it is the class which allows other class to write under its command
		QualisysConnection myQualisysConnection;
		// set true, if you want stream data and recording, set false if you don't want to record.
		myQualisysConnection.setRecord(true);
		myQualisysConnection.setDirectory(qualisys_outputdir);
		// specify this function if you want to control GUI capture
		myQualisysConnection.setControlGUICapture(qualisys_password);
		// by specifying command above, streaming mode automatically set to STREAM_USING_COMMAND
		// however, you can still specify how you want to stream here. for debugging purposes, i used STREAM_USING_NOTHING
		myQualisysConnection.setStreamingMode(QualisysConnection::STREAM_USING_NOTHING);

		// create a thread
		std::thread threadQualisys(std::ref(myQualisysConnection));
		// join with all other threads
		threadQualisys.join();
		printf("here");
	}
	else 
	{
		// if not using qualisys, directly start the synch
		synch::start();
	}


	if (useAMode) 
	{
		// create AmodeUSConnection object to connect with A-Mode Machine
		AModeUSConnection amodeUSConnection(amode_ip, amode_port, amode_mode);
		// set true if you want to stream data and recording
		amodeUSConnection.setRecord(true);
		amodeUSConnection.setDirectory(amode_outputdir);
		// set true if you also want to recieve index bytes from A-Mode machine (e.g. for indexing data)
		amodeUSConnection.useDataIndex(true);

		// create a thread
		std::thread threadAMode(std::ref(amodeUSConnection));
		// join with all other threads
		threadAMode.join();
	}

	if (useBMode) {
		// for the moment i dont put the b-mode
	}

	return 0;
}
