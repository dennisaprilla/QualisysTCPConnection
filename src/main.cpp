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

	// initalize value for Qualisys system
	std::string qualisys_ip = "127.0.0.1";
	unsigned short qualisys_port = 22222;
	std::string qualisys_password = "password";
	std::string qualisys_outputdir = "D:\\amodestream\\log";

	// initialize value for A-mode ultrasound system
	std::string amode_ip = "192.168.1.2";
	std::string amode_port = "6340";
	std::string amode_outputdir = "D:\\amodestream\\log";
	int amode_mode = 0;
	int amode_samples = 1500;
	int amode_probes = 30;


	// If not using qualisys, directly start the synch
	// There is a backstory behind this line. The goal is to run Qualisys and AMode connection in parallel, but, i also want
	// a feature that i can run AMode alone without using Qualisys. However, A-mode streaming is dependent to Qualisys because
	// Qualisys is the one who control every device to start. So the situation become quite tricky. There is a good simple
	// solution to this. That is:
	// 1) Check if the user want to use Qualisys, if true, initiate connection and create a thread within the IF, if otherwise
	//    start initiate synch::start() to allow other device to start.
	// 2) After the if block, initiate Amode connection and create a thread.
	// 3) When user is finished by pressing ESQ, the threads will be finished, and it will by catched by .join() function.
	// 
	// Since the qualisys thread will be created inside the IF block, we need to declare the thread (without doing anything)
	// before the if. Everything supposed to be going well... UNTIL, Qualisys thread somehow can't working well if the .join()
	// function is called OUTSIDE THE IF. WHAT A FUCKING NONSENSE. When I Debug it, there is something wrong with the send() 
	// function inside RTProtocol Class (a class which govern connection with Qualisys). I can't solve this. 
	//
	// Now, because Qualisys somehow requires .join() function within the same block, i can't put it above other code.
	// It will wait until the Qualisys thread is finished, so that it will block other code. Therefore, i put it below other
	// code. However, remember i also want a feature that "i can run AMode alone without using Qualisys"? it will be problematic.
	//
	// So, i create this one line block here, instead of in the else{} block from if(useQualisys) below. Everything is
	// working, and i don't want to touch this part anymore.
	if (!useQualisys) synch::start();

	// create AmodeUSConnection object to connect with A-Mode Machine
	AModeUSConnection amodeUSConnection(amode_ip, amode_port, amode_mode);
	// set true if you want to stream data and recording
	amodeUSConnection.setRecord(true);
	amodeUSConnection.setDirectory(amode_outputdir);
	// set true if you also want to recieve index bytes from A-Mode machine (e.g. for indexing data)
	amodeUSConnection.useDataIndex(true);
	// create a Amode thread 
	std::thread threadAMode(std::ref(amodeUSConnection));


	if (useQualisys)
	{
		// Qualisys is the center of the attention here, it is the class which allows other class to write under its command
		QualisysConnection myQualisysConnection;

		// setting up everything make sense if there is connection
		if (myQualisysConnection.getstatusQConnection()) {
			// set true, if you want stream data and recording, set false if you don't want to record.
			myQualisysConnection.setRecord(true);
			myQualisysConnection.setDirectory(qualisys_outputdir);
			// specify this function if you want to control GUI capture
			myQualisysConnection.setControlGUICapture(qualisys_password);
			// by specifying command above, streaming mode automatically set to STREAM_USING_COMMAND
			// however, you can still specify how you want to stream here. for debugging purposes, i used STREAM_USING_NOTHING
			myQualisysConnection.setStreamingMode(QualisysConnection::STREAM_USING_NOTHING);
			// create a thread for Qualisys
			std::thread threadQualisys(std::ref(myQualisysConnection));
			// stupid Qualisys class, i can't put the .join() function outside the if, what a nonsense. (refer to my speech above)
			threadQualisys.join();
		}
		else {
			synch::setStop(true);
		}
	}

	// join with all other threads
	threadAMode.join();

	return 0;
}
