// basic libraries
#include <stdio.h>
#include <string>
#include <vector>
#include <stdint.h>
#include <iostream>

// this library if for managing file
#include <filesystem>
#include <fstream>
#include <iterator>
#include <boost/filesystem.hpp>

// these libraries is for windows connection
#include <winsock2.h>
#include <ws2tcpip.h>
// need to connect the lib that is needed by winsock 
#pragma comment(lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

// Guillaume's libraries
// #include "Synch.h"
#include "getTime.h"
#include "Synch.h"

#include <opencv2/opencv.hpp>

#define DATA_RAW 0
#define DATA_DEPTH 1

#ifdef _WIN32
#define sleep Sleep
#else
#include <unistd.h>
#endif

/**
 * @brief AModeUSConnection used for handling the connection with A-mode Ultrasound Machine.
 */
class AModeUSConnection
{

private:
    // variables that stores the connection spec
    std::string ip_;                        //!< IP address of Ultrasound Machine
    std::string port_;                      //!< Port number of Ultrasound Machine
    SOCKET ConnectSocket_;                  //!< Socket which will be used for communication 


    // variables that stores amode spesifications
    int samples_;                           //!< The number of sample points in the signal (default 1500)
    int probes_;                            //!< The number of ultrasound probes being used in the experiment (default 30)
    int datalength_;                        //!< samples_ * probes_
    int headersize_ = 4;                    //!< The number of bytes of the header of the data packet
    int indexsize_;                         //!< The number of bytes which contains the information of the index (used for indexing)


    // variable that controls the behavior of this class
    bool setrecord_ = false;                //!< flag for setting the status of the record
    bool usedataindex_ = false;             //!< flag for using index 
    bool firstpass_ = true;                 //!< 
    int datamode_ = DATA_RAW;               //!< Mode to interpret data, DATA_RAW and DATA_DEPTH
    std::string fullpath_;                  //!< Full path to the directory where the data is stored
    std::string recorddirectory_;           //!< Local directory where the data is stored

    // for logging data
    std::ofstream ofs_;                     //!< Object for logging csv data

    // for testing
    int count_streameddata_ = 0;            //!< 
    int count_recordeddata_ = 0;             //!<
    std::stringstream filename_;            //!< 

public:

    /**
     * @brief Default constructor to built the AModeUSConnection.
     * I recommend to use this constructors, as everything is configured the way the ultrasound machine is.
     *
     * @param ip        IP address of the A-mode ultrasound machine.
     * @param port      Port of the A-mode ultrasound machine.
     * @param mode      Streaming mode, you can either choose DATA_RAW or DATA_DEPTH, depending of your scenario.
     */
    AModeUSConnection(std::string ip, std::string port, int mode);


    /**
     * @brief Second option for constructors, you can put your own sample number and probes.
     * Be careful if you want to use this contructors for initialization of the class,
     * you need to know the configuration from the ultrasound machine side, or else,
     * the data your retrieve will be very weird. This class doesn't include straming
     * mode because you are in control on how the data will be structured.
     *
     * @param ip        IP address of the A-mode ultrasound machine.
     * @param port      Port of the A-mode ultrasound machine.
     * @param samples   The number of point samples from the signals.
     * @param probes    The number of probes/transducers being used.
     */
    AModeUSConnection(std::string ip, std::string port, int samples, int probes);

    /**
     * @brief Destructor.
     */
    ~AModeUSConnection();


    /**
    * @brief A function to check if the PC and A-mode ultrasound is already connected through TCP.
    * @return              A flag indicating the status.
    */
    bool isConnected();


    /**
     * @brief A function to set the program to record the streamed A-mode signal.
     * @param flag          Set true to record.
     */
    void setRecord(bool flag);

    /**
     * @brief A function to set the program to name the streamed A-mode signal with index.
     * The data that is sent by A-mode Ultrasound Machine contains index, starting from 0 to 1.
     * This index is used for tracking if there any data loss, you can notice it if there is a
     * gap between the index. This is usefull for debugging the code.
     *
     * @param flag          Set true to use undex.
     */
    void useDataIndex(bool flag);


    /**
     * @brief A function to specify the where the streamed data will be stored.
     *
     * @param directory     Path to the directory.
     * @return              A flag indicating the status. -1 if there is something wrong.
     */
    int setDirectory(std::string directory);


    /**
     * @brief An alternative function to specify where the streamed data will be stored with custom file name.
     * This is used when you specify DATA_DEPTH for streaming mode, so that you can specify your own .csv file name.
     * However, you can't use this for DATA_RAW, since the naming is predetermined using timestamp and index.
     *
     * @param directory Path to directory.
     * @param filename Filename.
     * @return A flag indicating the status. -1 if there is something wrong.
     */
    int setDirectory(std::string directory, std::string filename);



    /**
     * @brief An experimental function for receiving data.
     * Please dont use this and just don't care. This is just testing code, but i am too affraid to delete it.
     *
     * @return A flag indicating the status. -1 if there is something wrong.
     */
    int receiveData();


    /**
     * @brief A function to receive the data for DATA_RAW mode.
     * Each data packet is a long array of unit16_t, with the number (samples_ * probes_).
     * Raw data will be converted into uint16_t then stored to .TIFF file. OpenCV is used in
     * this process since they are very effective in storing big image.
     *
     * @param ultrasound_frd    A pointer to array of unit16_t where the A-mode Ultrasound raw signal is stored.
     * @param receivebuffer     A temporary buffer that we will use for store binary string from socket.
     *                          The A-mode ultrasound machine always send full data (header+index+data)
     *                          so we need to accomodate the full data.
     * @param receivebuffersize The byte size of recievebuffer (all, header+index+data).
     * @param datasize          The byte size of data (only data without header and index).
     * @param headerindexsize   This will be used to skip or not to skip the bytes that represent the
     *                          index so if the user specified useDataIndex(true), it means we do not skip the index
     *                          byte thus the headerindexsize is only the size of headersize_
     * @return                  A flag indicating the status. >0 means there is data, 0 means the connection is closed, -1 means there is something wrong.
     */
    int receiveData(std::vector<uint16_t>* ultrasound_frd, char* receivebuffer, int receivebuffersize, int datasize, int headerindexsize);
    

    /**
     * @brief A function to receive the data for DATA_DEPTH mode.
     * Each data packet is a long array of double, with the number of probes_. Depth data will be stored in .csv file.
     *
     * @param ultrasound_frd    A pointer to array of unit16_t where the A-mode Ultrasound raw signal is stored.
     * @param receivebuffer     A temporary buffer that we will use for store binary string from socket.
     *                          The A-mode ultrasound machine always send full data (header+index+data)
     *                          so we need to accomodate the full data.
     * @param receivebuffersize The byte size of recievebuffer (all, header+index+data).
     * @param datasize          The byte size of data (only data without header and index).
     * @param headerindexsize   This will be used to skip or not to skip the bytes that represent the
     *                          index so if the user specified useDataIndex(true), it means we do not skip the index
     *                          byte thus the headerindexsize is only the size of headersize_
     * @return                  A flag indicating the status. >0 means there is data, 0 means the connection is closed, -1 means there is something wrong.
     */
    int receiveData(std::vector<double>* ultrasound_frd, char* receivebuffer, int receivebuffersize, int datasize, int headerindexsize);

    /**
     * @brief A function that is used for multithreading.
     * Here, receiveData() will be invoked. Several data specification is configured also in this funuction.
     *
     */
    void operator()();


    enum enumMessageType {
        MESSAGE_OK,
        MESSAGE_WARNING,
        MESSAGE_RUNNING
    };

    /**
     * @brief A print function which specify the source device (A-Mode) and type of message.
     *
     * There are 3 types of message: MESSAGE_OK [OK], MESSAGE_WARNING [!!], MESSAGE_RUNNING [>>].
     */
    void myprintFormat(AModeUSConnection::enumMessageType messagetype, std::string message);



    bool userquit_ = false;                     //!< A flag which specified if the user wants to exit

protected:


    /**
     * @brief Initialization of TCP Socket connection.
     * To understand more about this please follow the official tutorial from microsoft,
     * which can be found in. https://docs.microsoft.com/en-us/windows/win32/winsock/getting-started-with-winsock
     *
     * @param ConnectSocket     Pointer to socket object, which will be used in the entire code for data streaming.
     * @return                  A flag indicating the status. 0 if connection is successfully made, -1 if there is something wrong.
     */
    int connectTCP(SOCKET* ConnectSocket);

    /**
     * @brief If user pressed ESC, program halts and finished
    */
    bool checkKeyPressed()
    {
        return (GetKeyState(VK_ESCAPE) & 0x8000);
    }
};