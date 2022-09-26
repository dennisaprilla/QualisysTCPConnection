#pragma once

#include <iostream>
#include <string>

// library from qualisys
// https://github.com/qualisys/qualisys_cpp_sdk
#include "RTProtocol.h"
#include "RTPacket.h"

// a class that inherit OpenSimFileLogger (from guillaume)
#include "QualisysLogger.h"
// a class for mantaining synchronization start and stop for all devices
#include "Synch.h"
#include "getTime.h"

#include "Eigen/Dense"

// library from boost, to manage file
// https://www.boost.org/users/download/
#include <boost/filesystem.hpp>


#ifdef _WIN32
#define sleep Sleep
#else
#include <unistd.h>
#endif

class QualisysConnection
{


public:

    /**
     * @brief Default constructor.
     * This constructor uses local ip address and default port 22222.
     * 
    */
	QualisysConnection();

    /**
     * @brief Constructor with Qualisys ip and port specified.
     * 
     * @param ip IP address to the PC which connected to Qualisys Motion Capture System
     * @param port Port number for connection.
    */
    QualisysConnection(std::string ip, unsigned short port);
	~QualisysConnection();


    /**
     * @brief Set function to take Qulisys GUI control
     * @return flag indicating the successs of controlling.
    */
    int setControlGUIRecord(std::string password);

    /**
     * @brief Set the directory for storing the logs.
     * @param recordDirectory Directory name.
    */
    void setDirectory(const std::string directory)
    {
        recordDirectory_ = directory;
    }

    /**
     * @brief Get the directory for storing the logs.
    */
    std::string getDirectory()
    {
        return recordDirectory_;
    }

    /**
     * @brief Set function to record the data
     * @param flag boolean.
    */
    void setRecord(bool flag)
    {
        record_ = flag;
    }

    /**
     * @brief Get recording status
    */
    bool getRecord()
    {
        return record_;
    }

    /**
     * @brief Overloading operator().
     * 
     * This operator will be called when we want to pass the class to the thread
    */
    void operator()();




protected:

    /**
     * @brief Initializing connection to Qualisys, called in constructor.
     * 
     * @return 0 success, -1 error occured.
    */
    int connectTCP();

    /**
     * @brief Reading the marker settings from Qualisys.
     * Retrieve the name of the rigid bodies that is detected by Qualisis. Will be used for logging.
     * 
     * @return 0 success, -1 error occured.
    */
    int readMarkerSettings();

    /**
     * @brief Receiving data (6DoF data and event) from Qualisys.
    */
    int receiveData();

    /**
     * @brief If user pressed ESC, program halts and finished
    */
    bool checkKeyPressed()
    {
        return (GetKeyState(VK_ESCAPE) & 0x8000);
    }


    CRTProtocol poRTProtocol_;          //!< Class for the communication with the Qualisys software.




private:

    std::string    ip_   = "127.0.0.1";      //!< Default IP address
    unsigned short port_ = 22222;            //!< Default port number (22222).

    const int      majorVersion = 1;         //!< Qualisys major version used for connecting to Qualisys (1).
    const int      minorVersion = 19;        //!< Qualisys minor version used for connecting to Qualisys (19).
    const bool     bigEndian = false;        //!< Default order of sequence (false).
    unsigned short udpPort = 6734;           //!< Default udp port (6734).


    bool record_ = false;                    //!< A flag to record (false).
    std::string recordDirectory_;            //!< Directory for recording data.

    bool controlGUIrecord_ = false;          //!< A flag to record with GUI (false).
    std::string controlPassword_;            //!< A password for controling Qualisys GUI.

    double timeStamp_;                       //!< timestamp when a data arrived to PC (in seconds).
	double timeStampQualisys_;               //!< timestamp from Qualisys Data Packet converted (in seconds).
    std::vector<std::string> rigidbodyName_; //!< Contains list of rigidbody names.
    std::vector<double> rigidbodyData_;      //!< Contains value of rigidbodies.
    QualisysLogger* logger_;                 //!< A class for managing logging, inherited from OpenSimFileLogger


    bool userquit_ = false;                   //!< A flag which specified if the user wants to exit
    bool userstart_ = false;                  //!< A flag which specified if the user wants to start

};
