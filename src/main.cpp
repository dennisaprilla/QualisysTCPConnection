// TestQualisys7.cpp : Defines the entry point for the application.
//

#include <iostream>
#include <thread>

#include "Synch.h"
#include "QualisysConnection.h"
#include "AModeUSConnection.h"

// dependencies
#include <tclap/CmdLine.h>

// function for parsing arguments
void commandLineOptions(const int& argc, char** argv,
	bool& useQualisys, std::string& outputdir,
	std::string& qualisys_ip, unsigned short& qualisys_port, std::string& qualisys_pass,
	std::string& amode_ip, std::string& amode_port,
	int& amode_mode, int& amode_samples, int& amode_probes) {

	// see TCLAP (Templatized C++ Command Line Parser Manual) documentation
	// can be found in: http://tclap.sourceforge.net/manual.html
	try {
		// Define the command line object, and insert a message
		// that describes the program. The "Command description message" 
		// is printed last in the help text. The second argument is the 
		// delimiter (usually space) and the last one is the version number. 
		// The CmdLine object parses the argv array based on the Arg objects
		// that it contains. 
		TCLAP::CmdLine cmd("Command description message", ' ', "1.0");

		// Define a value argument and add it to the command line.
		// A value arg defines a flag and a type of value that it expects,
		// such as "-n Bishop".
		// General
		TCLAP::ValueArg<std::string> nameargOutputdir("o", "outputdir", "Output directory to store A-mode and Qualisys data", false, "", "string");
		// Qualisys
		TCLAP::ValueArg<std::string> nameargQIP("", "Qip", "IP address of Qualisys sytem", false, "127.0.0.1", "string");
		TCLAP::ValueArg<unsigned short> nameargQPort("", "Qport", "Port number of Qualisys sytem", false, 22222, "unsigned short");
		TCLAP::ValueArg<std::string> nameargQPass("", "Qpass", "IP address of Qualisys sytem. Check the Qualisys settings to know the password.", false, "password", "string");
		// A-mode
		TCLAP::ValueArg<std::string> nameargAIP("", "Aip", "IP address of A-mode Ultrasound System PC", true, "192.168.0.2", "string");
		TCLAP::ValueArg<std::string> nameargAPort("", "Aport", "Port number of A-mode Ultrasound System PC", true, "6340", "string");
		TCLAP::ValueArg<int> nameargAModeMode("m", "mode", "A-Mode data mode. You can choose between raw data or depth data streaming, depends on the configuration you choose from A-mode Ultrasound PC. Specify 0 for raw, 1 for depth.", false, 0, "int");
		TCLAP::ValueArg<int> nameargAModeSamples("n", "samples", "Number of samples of A-Mode Signal. If you want to see deep inside the soft tissue, put bigger value. However, if you put too big, it can affect to streaming speed performance.", false, 1500, "int");
		TCLAP::ValueArg<int> nameargAModeProbes("p", "probes", "Number of probes of A-Mode Signal, depends on the physical setup you have from A-mode Ultrasound machine. Our current setup is 30 probes.", false, 30, "int");

		// Add the argument nameArg to the CmdLine object. The CmdLine object
		// uses this Arg to parse the command line.
		cmd.add(nameargOutputdir);
		cmd.add(nameargQIP);
		cmd.add(nameargQPort);
		cmd.add(nameargQPass);
		cmd.add(nameargAIP);
		cmd.add(nameargAPort);
		cmd.add(nameargAModeMode);
		cmd.add(nameargAModeSamples);
		cmd.add(nameargAModeProbes);

		// Define a switch and add it to the command line.
		// A switch arg is a boolean argument and only defines a flag that
		// indicates true or false.  In this example the SwitchArg adds itself
		// to the CmdLine object as part of the constructor.  This eliminates
		// the need to call the cmd.add() method.  All args have support in
		// their constructors to add themselves directly to the CmdLine object.
		// It doesn't matter which idiom you choose, they accomplish the same thing.
		TCLAP::SwitchArg nameargUseQualisys("q", "qualisys", "Use Qualisys, makes it A-mode+Qualisys system.", cmd, false);

		// Parse the argv array.
		cmd.parse(argc, argv);

		// Get the value parsed by each arg.
		// general
		useQualisys = nameargUseQualisys.getValue();
		outputdir = nameargOutputdir.getValue();
		// qualisys
		qualisys_ip = nameargQIP.getValue();
		qualisys_port = nameargQPort.getValue();
		qualisys_pass = nameargQPass.getValue();
		// amode
		amode_ip = nameargAIP.getValue();
		amode_port = nameargAPort.getValue();
		amode_mode = nameargAModeMode.getValue();
		amode_samples = nameargAModeSamples.getValue();
		amode_probes = nameargAModeProbes.getValue();

	}
	catch (TCLAP::ArgException& e)  // catch exceptions
	{
		std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	}

}


int main(int argc, char** argv)
{
	// welcome message
	std::cout << "Qualisys + Ultrasound TCP Connection." << std::endl;
	std::cout << "D.A. Christie & G. Durandau, University of Twente." << std::endl << std::endl;

	// initialize general parameter for the program
	bool useQualisys = false;
	bool isRecord = false;
	std::string outputdir = "";

	// initalize value for Qualisys system
	std::string qualisys_ip = "127.0.0.1";
	unsigned short qualisys_port = 22222;
	std::string qualisys_password = "password";

	// initialize value for A-mode ultrasound system
	std::string amode_ip = "192.168.1.2";
	std::string amode_port = "6340";
	int amode_mode = 0;
	int amode_samples = 2500;
	int amode_probes = 30;

	// parse the arguments from command line and store it to our variables
	commandLineOptions(argc, argv, 
		useQualisys, outputdir,
		qualisys_ip, qualisys_port, qualisys_password,
		amode_ip, amode_port, amode_mode, amode_samples, amode_probes);

	// if the user specified the record directory, it means they want to record
	if (!outputdir.empty()) isRecord = true;
	else outputdir = "D:\\amodestream\\log"; // set default directory

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
	amodeUSConnection.setRecord(false);
	amodeUSConnection.setDirectory(outputdir);
	// set true if you also want to recieve index bytes from A-Mode machine (e.g. for indexing data)
	amodeUSConnection.useDataIndex(true);
	// create a Amode thread 
	std::thread threadAMode(std::ref(amodeUSConnection));


	if (useQualisys)
	{
		// Qualisys is the center of the attention here, it is the class which allows other class to write under its command
		QualisysConnection myQualisysConnection(qualisys_ip, qualisys_port);

		// setting up everything make sense if there is connection
		if (myQualisysConnection.getstatusQConnection()) {
			// if qualisys running, change the amodeUSconnection record status to true;
			// something strange with the connection is happening when i directly set the record to true
			// especially when qualisys is not connected. The second time this program is run, connection
			// to A-mode cannot be established somehow. SUPER WEIRD!!!!
			amodeUSConnection.setRecord(isRecord);

			// set true, if you want stream data and recording, set false if you don't want to record.
			myQualisysConnection.setRecord(isRecord);
			myQualisysConnection.setDirectory(outputdir);
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
			// need to be like this, or else, in the next connection A-mode wont stream. SUPER WEIRD!!!!
			//synch::start();
			//sleep(1000);
			synch::setStop(true);
		}
	}
	else {
		// because i set this false in the beginning of the code, now i need to turn this to true.
		// This is just because something strange in second-connection, which is i don't know what is actually happening.
		amodeUSConnection.setRecord(isRecord);
	}

	// join with all other threads
	threadAMode.join();

	// closing message
	std::cout << std::endl << "Program Closed." << std::endl;

	return 0;
}
