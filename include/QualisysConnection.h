#pragma once

// basic libraries
#include <iostream>
#include <string>

// library from qualisys
// https://github.com/qualisys/qualisys_cpp_sdk
#include "RTProtocol.h"
#include "RTPacket.h"

// a class that inherit OpenSimFileLogger for logging data (from guillaume)
#include "QualisysLogger.h"
// a class for maintaining synchronization start and stop for all devices
#include "Synch.h"
#include "getTime.h"
// a class for converting rotation matrix to quaternion (to reduce data size)
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
    int setControlGUICapture(std::string password);

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
     * @brief Get connection status
    */
    bool getstatusQConnection() {
        return statusQConnection_;
    }

    /**
     * @brief Get marker setting status
    */
    bool getstatusQMarker() {
        return statusQMarker_;
    }

    /**
     * @brief Overloading operator().
     * 
     * This operator will be called when we want to pass the class to the thread
    */
    void operator()();


    enum enumStreamModes {
        STREAM_USING_NOTHING,
        STREAM_USING_MANUAL_BUTTON,
        STREAM_USING_COMMAND
    };

    enum enumMessageType {
        MESSAGE_OK,
        MESSAGE_WARNING,
        MESSAGE_RUNNING
    };

    /**
     * @brief Set function to specify which streaming mode is used
     *
     * There are 3 modes currently available: 
     * STREAM_USING_MANUAL_BUTTON is used if the user wants to press the capture button manually from QTM GUI.
     * STREAM_USING_COMMAND is used if the user wants everything started from this class, without intervention in QTM GUI.
     * STREAM_USING_NOTHING usually used for debugging, it will make the program directly stream data.
     * 
    */
    void setStreamingMode(QualisysConnection::enumStreamModes mode)
    {
        streamMode_ = mode;
    }

    /**
     * @brief A print function which specify the source device (Qualisys) and type of message.
     * 
     * There are 3 types of message: MESSAGE_OK [OK], MESSAGE_WARNING [!!], MESSAGE_RUNNING [>>].
     */
    void myprintFormat(QualisysConnection::enumMessageType messagetype, std::string message);

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

    std::string    ip_   = "127.0.0.1";         //!< Default IP address
    unsigned short port_ = 22222;               //!< Default port number (22222).

    const int      majorVersion = 1;            //!< Qualisys major version used for connecting to Qualisys (1).
    const int      minorVersion = 19;           //!< Qualisys minor version used for connecting to Qualisys (19).
    const bool     bigEndian = false;           //!< Default order of sequence (false).
    unsigned short udpPort = 6734;              //!< Default udp port (6734).

    bool statusQConnection_ = false;
    bool statusQMarker_     = false;

    bool record_ = false;                       //!< A flag to record (false).
    std::string recordDirectory_;               //!< Directory for recording data.

    enumStreamModes streamMode_ = QualisysConnection::STREAM_USING_MANUAL_BUTTON; //!< Stream mode to decide how the data will be streamed
    std::string controlPassword_;               //!< A password for controling Qualisys GUI.

    double timeStamp_;                          //!< timestamp when a data arrived to PC (in seconds).
	double timeStampQualisys_;                  //!< timestamp from Qualisys Data Packet converted (in seconds).
    std::vector<std::string> rigidbodyName_;    //!< Contains list of rigidbody names.
    std::vector<double> rigidbodyData_;         //!< Contains value of rigidbodies.
    QualisysLogger* logger_;                    //!< A class for managing logging, inherited from OpenSimFileLogger


    bool userquit_ = false;                     //!< A flag which specified if the user wants to exit
    bool userstart_ = false;                    //!< A flag which specified if the user wants to start

};
