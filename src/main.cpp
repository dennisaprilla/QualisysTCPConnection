// TestQualisys7.cpp : Defines the entry point for the application.
//

#include <iostream>
#include "QualisysConnection.h"
#include "Synch.h"


int main()
{
	bool startWithoutQualisysCapture = false;
	if (startWithoutQualisysCapture) synch::start();

	// Qualisys is the center of the attention here, it is the class which allows other
	// class to write under its command
	QualisysConnection myQualisysConnection;

	myQualisysConnection.setRecord(true);
	myQualisysConnection.setDirectory("D:/qualisyslog");

	std::thread threadQualisys(std::ref(myQualisysConnection));
	threadQualisys.join();

	return 0;
}
