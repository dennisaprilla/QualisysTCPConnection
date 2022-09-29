// TestQualisys7.cpp : Defines the entry point for the application.
//

#include <iostream>
#include "QualisysConnection.h"
#include "Synch.h"

// dependencies
#include <tclap/CmdLine.h>

// the main class that we will use for streaming
#include "AModeUSConnection.h"


int main()
{
	bool useQualisys = true;
	bool useAMode    = true;
	bool useBMode	 = true;

	if (useAMode) {

	}

	if (useBMode) {

	}

	if (useQualisys) {

		// Qualisys is the center of the attention here, it is the class which allows other class to write under its command
		QualisysConnection myQualisysConnection;
		// set true, if you want only stream data and recording, set false if you don't want to record.
		myQualisysConnection.setRecord(true);
		myQualisysConnection.setDirectory("D:/qualisyslog");
		// specify this function if you want to control GUI capture
		myQualisysConnection.setControlGUICapture("password");
		// by specifying command above, streaming mode automatically set to STREAM_USING_COMMAND
		// however, you can still specify how you want to stream here. for debugging purposes, i used STREAM_USING_NOTHING
		myQualisysConnection.setStreamingMode(QualisysConnection::STREAM_USING_NOTHING);


		std::thread threadQualisys(std::ref(myQualisysConnection));
		threadQualisys.join();
	}

	return 0;
}
