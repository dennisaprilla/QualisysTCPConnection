#include "AModeUSConnection.h"

AModeUSConnection::AModeUSConnection(std::string ip, std::string port, int mode) {
    ip_ = ip;
    port_ = port;
    ConnectSocket_ = INVALID_SOCKET;

    switch (mode) {
        // this mode will encode the received data as raw data
        case DATA_RAW:
            probes_ = 30;
            samples_ = 1500;
            datamode_ = DATA_RAW;
            indexsize_ = 2;
            break;

        // this mode will encode the received data as depth data
        case DATA_DEPTH:
            probes_ = 30;
            samples_ = 2;
            datamode_ = DATA_DEPTH;
            indexsize_ = 8;
            break;
        }
    datalength_ = samples_ * probes_;

    connectTCP(&ConnectSocket_);

}


AModeUSConnection::AModeUSConnection(std::string ip, std::string port, int samples, int probes) {
    ip_ = ip;
    port_ = port;
    ConnectSocket_ = INVALID_SOCKET;

    samples_ = samples;
    probes_ = probes;
    datalength_ = samples_ * probes_;

    connectTCP(&ConnectSocket_);
}

AModeUSConnection::~AModeUSConnection() {
    //shutdown(ConnectSocket_, SD_SEND);
    //closesocket(ConnectSocket_);
    //WSACleanup();

    //synch::start();
    //sleep(1);
    //synch::setStop(true);
}


int AModeUSConnection::connectTCP(SOCKET* ConnectSocket) {
    // variable to store the status flag
    int iResult;

    // STEP 1: INITIALIZE WINSOCK
    WSADATA wsaData;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        char strbuffer[100];
        sprintf(strbuffer, "WSAStartup failed: %ld", WSAGetLastError());
        myprintFormat(AModeUSConnection::MESSAGE_WARNING, strbuffer);
        return -1;
    }

    // STEP 2: CREATING THE SOCKET
    struct addrinfo* result, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo(ip_.c_str(), port_.c_str(), &hints, &result);
    if (iResult != 0) {
        char strbuffer[100];
        sprintf(strbuffer, "getaddrinfo failed: %ld", WSAGetLastError());
        myprintFormat(AModeUSConnection::MESSAGE_WARNING, strbuffer);
        WSACleanup();
        return -1;
    }

    *ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (*ConnectSocket == INVALID_SOCKET) {
        char strbuffer[100];
        sprintf(strbuffer, "Error at socket(): %ld", WSAGetLastError());
        myprintFormat(AModeUSConnection::MESSAGE_WARNING, strbuffer);

        freeaddrinfo(result);
        WSACleanup();
        return -1;
    }

    // STEP 3: CONNECT TO THE SOCKET
    iResult = connect(*ConnectSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(*ConnectSocket);
        *ConnectSocket = INVALID_SOCKET;
    }

    freeaddrinfo(result);

    if (*ConnectSocket == INVALID_SOCKET) {
        myprintFormat(AModeUSConnection::MESSAGE_WARNING, "Unable to connect to server!");
        WSACleanup();
        return -1;
    }

    char strbuffer[100];
    sprintf(strbuffer, "Connected to the A-Mode Ultrasound Machine, specified at ip: %s, port: %s", ip_.c_str(), port_.c_str());
    myprintFormat(AModeUSConnection::MESSAGE_OK, strbuffer);
    return 0;
}



bool AModeUSConnection::isConnected() {
    if (ConnectSocket_ != INVALID_SOCKET) return true;
    else return false;
}


void AModeUSConnection::setRecord(bool flag) {

    setrecord_ = flag;
}


void AModeUSConnection::useDataIndex(bool flag) {

    usedataindex_ = flag;
}



int AModeUSConnection::setDirectory(std::string directory) {
    // check if the directory is exists
    if (!boost::filesystem::exists(directory)) {
        if (!boost::filesystem::create_directories(directory)) {
            myprintFormat(AModeUSConnection::MESSAGE_WARNING, "Unable to create directory for A-mode Ultrasound logging");
            return -1;
        }
    }

    // if user specified DATA_DEPTH, we will save it in csv file, so we need to open the stream first
    // contrast to raw data, it will be saved in .tiff file using opencv imwrite function while streaming (at least for now)
    if (datamode_ == DATA_DEPTH) {

        // prepare the file name
        std::string filename = std::to_string(rtb::getTime());
        if (directory.back() == '\\') fullpath_ = directory + filename + ".csv";
        else fullpath_ = directory + "\\" + filename + ".csv";

        // open the file stream
        ofs_.open(fullpath_);
    }

    recorddirectory_ = directory;
    return 0;
}


int AModeUSConnection::setDirectory(std::string directory, std::string filename) {

    // check if the directory is exists
    if (!boost::filesystem::exists(directory)) {
        if (!boost::filesystem::create_directories(directory)) {
            myprintFormat(AModeUSConnection::MESSAGE_WARNING, "Unable to create directory for A-mode Ultrasound logging");
            return -1;
        }
    }

    // if everything goes fine open the csv file
    if (directory.back() == '\\') fullpath_ = directory + filename + ".csv";
    else fullpath_ = directory + "\\" + filename + ".csv";

    ofs_.open(fullpath_);

    return 0;
}

// An experimental function for receiving data. Please dont use this and just don't care. 
// This is just testing code, but i am too affraid to delete it.
int AModeUSConnection::receiveData() {
    int iResult;

    // we should expect how much the size of the data we will get
    //const int datasize = sizeof(int16_t) * datalength_ + header_;
    const int datasize = sizeof(int16_t) * datalength_ + headersize_;

    // construct a vector where we will put the data
    //std::vector<int16_t> ultrasound_frd;
    //ultrasound_frd.resize(datasize / sizeof(int16_t));
    std::vector<uint16_t> ultrasound_frd;
    ultrasound_frd.resize((datasize - headersize_) / sizeof(uint16_t));

    // temporary buffer that we will use for store binary string from socket
    char* receivebuffer = new char[datasize];

    count_streameddata_ = 0;
    double timestamp = rtb::getTime();

    do {

        // read data from socket, put it in temporary variable
        iResult = recv(ConnectSocket_, receivebuffer, datasize, 0);

        // i just affraid there will be corrupted data so i put iResult==datasize here
        if (iResult > 0) {

            if (iResult == datasize) {

                // https://stackoverflow.com/questions/52492229/c-byte-array-to-int
                memcpy(ultrasound_frd.data(), receivebuffer + headersize_, datasize);

                for (auto i = ultrasound_frd.begin(); i != ultrasound_frd.end(); ++i) {
                    printf("%d ", *i);
                }

                // double timestamp = rtb::getTime();

                //ofs_ << std::to_string(rtb::getTime()) << ",";
                //std::copy(ultrasound_frd.begin(), ultrasound_frd.end(), std::ostream_iterator<int16_t>(ofs_, ","));
                //ofs_ << "\n";

                // for (auto it = ultrasound_frd.begin(); it != ultrasound_frd.end(); it++) ofs_ << *it << ",";
                // ofs_ << "\n";

                //filename_.str(std::string());
                //filename_ << recorddirectory_ << "\\" << std::to_string(rtb::getTime()) << ".tiff";

                //cv::Mat amodeimage = cv::Mat(ultrasound_frd).reshape(1, 30);
                //cv::imwrite(filename_.str(), amodeimage);

                printf("(%dB)\n", iResult);

                count_streameddata_++;
            }
        }

        // 0 means connection is closed 
        else if (iResult == 0)
            printf("Connection closed\n");

        // -1 means something happened with the connection    
        else
            printf("recv failed: %d\n", WSAGetLastError());


    } while (iResult > 0);

    double timestamp2 = rtb::getTime();
    std::cout << timestamp2 << " - " << timestamp << " = " << timestamp2 - timestamp << " (" << (timestamp2 - timestamp) / count_streameddata_ << ")\n";

    // STEP 5: DISCONNECT
    iResult = shutdown(ConnectSocket_, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket_);
        WSACleanup();
        return -1;
    }

    closesocket(ConnectSocket_);
    WSACleanup();

    return 0;
}


// A function to receive the data for DATA_RAW mode.
int AModeUSConnection::receiveData(std::vector<uint16_t>* ultrasound_frd, char* receivebuffer, int receivebuffersize, int datasize, int headerindexsize) {

    // read data from socket, put it in temporary variable
    int iResult = recv(ConnectSocket_, receivebuffer, receivebuffersize, 0);

    // if >0 it means there is something in the socket, we need to read it
    if (iResult > 0) {

        // i just affraid there will be corrupted data so i checked here
        if (iResult == receivebuffersize) {

            // convert binary string to something else without loop
            // https://stackoverflow.com/questions/52492229/c-byte-array-to-int
            // https://en.cppreference.com/w/cpp/string/byte/memcpy
            memcpy(ultrasound_frd->data(), receivebuffer + headerindexsize, datasize);

            // lets print the bytes, not really neccessary actually
            // printf("Amode : (%dB)\n", iResult);

            // record only when the user stated that he wants to record
            if (setrecord_) {

                // if this is first data, wait until trigger from qualisys before i write to a file
                if (firstpass_) {

                    // let's put a message here, and wait...
                    myprintFormat(AModeUSConnection::MESSAGE_RUNNING, "Waiting for Qualisys to start to record A-Mode Raw Data.");
                    synch::waitStart();

                    // We can get stop signal from the QualisysConnection eventhough we are not starting yet. This is when
                    // the program somehow can't connect to Qualisys. To prevent the AModeConnection goes further, let's block
                    // it right away here.
                    //if (synch::getStop()) return 0;
                    // if not we continue recording
                    //else 
                    myprintFormat(AModeUSConnection::MESSAGE_RUNNING, "Start recording A-Mode Raw Data.");

                    // put the flag false so that we will not go to this block anymore.
                    firstpass_ = false;
                }

                // creating a string for the name of the file
                filename_.str(std::string());
                if (usedataindex_) {

                    // if using data index, the filename structured as <timestamp>_<index>.tiff
                    filename_ << recorddirectory_ << "\\"
                        << std::to_string(rtb::getTime())
                        << "_" << (int16_t)*ultrasound_frd->data()
                        << ".tiff";

                    // In a moment, i use opencv to transform our long array to matrix then save it as a .tiff image.
                    // I tried to save it to .csv but somehow it perform very slowly,
                    // i think there is an interference with some functions somewhere, but idk.
                    // This one is working well so i will stick to this.
                    std::vector<uint16_t> tmp(ultrasound_frd->begin() + 1, ultrasound_frd->end());
                    cv::Mat amodeimage = cv::Mat(tmp).reshape(1, 30);
                    cv::imwrite(filename_.str(), amodeimage);
                }

                else {
                    // if not, only <timestamp>.tiff
                    filename_ << recorddirectory_ << "\\"
                        << std::to_string(rtb::getTime())
                        << ".tiff";

                    // same explanation above
                    cv::Mat amodeimage = cv::Mat(*ultrasound_frd).reshape(1, 30);
                    cv::imwrite(filename_.str(), amodeimage);
                }


                count_recordeddata_++;
            }

            count_streameddata_++;
        }
    }

    // 0 means connection is closed 
    else if (iResult == 0) {
        myprintFormat(AModeUSConnection::MESSAGE_WARNING, "Connection closed from A-Mode US Machine.");
        synch::setStop(true);
    }

    // -1 means something happened with the connection
    else {
        char strbuffer[100];
        sprintf(strbuffer, "recv() failed from A-Mode US Machine: %ld", WSAGetLastError());
        myprintFormat(AModeUSConnection::MESSAGE_WARNING, strbuffer);
        synch::setStop(true);
    }

    // check if user presed a key
    userquit_ = this->checkKeyPressed();

    return iResult;
}


// A function to receive the data for DATA_DEPTH mode.
int AModeUSConnection::receiveData(std::vector<double>* ultrasound_frd, char* receivebuffer, int receivebuffersize, int datasize, int headerindexsize) {

    // read data from socket, put it in temporary variable
    int iResult = recv(ConnectSocket_, receivebuffer, receivebuffersize, 0);

    // if >0 it means there is something in the socket, we need to read it
    if (iResult > 0) {

        // i just affraid there will be corrupted data so i put iResult==datasize here
        if (iResult == receivebuffersize) {

            // convert binary string to something else without loop
            // https://stackoverflow.com/questions/52492229/c-byte-array-to-int
            memcpy(ultrasound_frd->data(), receivebuffer + headerindexsize, datasize);

            // lets print the bytes, not really neccessary actually
            // printf("Amode : (%dB)\n", iResult);

            // record only when the user stated that he wants to record
            if (setrecord_) {

                // if this is first data, wait until trigger from qualisys before i write to a file
                if (firstpass_) {

                    // let's put a message here, and wait...
                    myprintFormat(AModeUSConnection::MESSAGE_RUNNING, "Waiting for Qualisys to start to record A-Mode Depth Data.");
                    synch::waitStart();

                    // We can get stop signal from the QualisysConnection eventhough we are not starting yet. This is when
                    // the program somehow can't connect to Qualisys. To prevent the AModeConnection goes further, let's block
                    // it right away here.
                    if (synch::getStop()) return 0;
                    // if not we continue recording
                    else myprintFormat(AModeUSConnection::MESSAGE_RUNNING, "Start recording A-Mode Raw Data.");

                    // put the flag false so that we will not go to this block anymore.
                    firstpass_ = false;
                }

                // first column is timestamp
                ofs_ << std::to_string(rtb::getTime()) << ",";
                // write to csv in style, to make sure it is faster
                std::copy(ultrasound_frd->begin(), ultrasound_frd->end(), std::ostream_iterator<double>(ofs_, ","));
                ofs_ << "\n";


                count_recordeddata_++;
            }

            count_streameddata_++;
        }
    }

    // 0 means connection is closed 
    else if (iResult == 0) {
        myprintFormat(AModeUSConnection::MESSAGE_WARNING, "Connection closed from A-Mode US Machine.");
        synch::setStop(true);
    }

    // -1 means something happened with the connection
    else {
        char strbuffer[100];
        sprintf(strbuffer, "recv() failed from A-Mode US Machine: %ld", WSAGetLastError());
        myprintFormat(AModeUSConnection::MESSAGE_WARNING, strbuffer);
        synch::setStop(true);
    }

    // check if user presed a key
    if (this->checkKeyPressed()) {
        myprintFormat(AModeUSConnection::MESSAGE_WARNING, "User pressed ESC. Stop recording");
        userquit_ = true;
    }

    return iResult;
}


/**
 * @brief A function that is used for multithreading.
 * Here, receiveData() will be invoked. Several data specification is configured also in this funuction.
 * 
 */
void AModeUSConnection::operator()() {

    int iResult;
    double timestamp = 0.0;

    if (datamode_ == DATA_RAW) {

        // construct a vector where we will put the data
        std::vector<uint16_t> ultrasound_frd;

        int headerindexsize = 0;
        int datavectorsize = 0;
        int datasize = 0;
        if (usedataindex_) {
            // headerindexsize will be used to skip or not to skip the bytes that represent the index
            // so if the user specified useDataIndex(true), it means we do not skip the index byte
            // thus the headerindexsize is only the size of headersize_
            headerindexsize = headersize_;

            // we should expect how much the size of the data we will get
            // ultrasound_frd needs to be resized with the index byte
            datasize = indexsize_ + (sizeof(uint16_t) * datalength_);
            ultrasound_frd.resize(datasize / sizeof(uint16_t));
        }

        else {
            // if the user specified otherwise, it means we skip the index byte
            // thus the headerindexsize is the total of headersize_ + indexsize_
            headerindexsize = headersize_ + indexsize_;

            // ultrasound_frd needs to be resized with the index byte
            datasize = 0 + (sizeof(uint16_t) * datalength_);
            ultrasound_frd.resize(datasize / sizeof(uint16_t));
        }

        // temporary buffer that we will use for store binary string from socket
        // the A-mode ultrasound machine always send full data (header+index+data)
        // so we need to accomodate the full data
        int receivebuffersize = (sizeof(uint16_t) * datalength_) + headersize_ + indexsize_;
        char* receivebuffer = new char[receivebuffersize];
        int bytereceived = 0;

        count_streameddata_ = 0;
        count_recordeddata_ = 0;
        timestamp = rtb::getTime();

        myprintFormat(AModeUSConnection::MESSAGE_RUNNING, "Start streaming A-mode US Raw Data.");
        // main loop to receive the data,
        // we will do this until there is an error or one of the system is stop
        do {
            bytereceived = receiveData(&ultrasound_frd, receivebuffer, receivebuffersize, datasize, headerindexsize);
        } while (bytereceived > 0 && !synch::getStop() && !userquit_);
        //} while (bytereceived > 0 && !userquit_);
    }

    else if (datamode_ == DATA_DEPTH) {

        // construct a vector where we will put the data
        std::vector<double> ultrasound_frd;

        int headerindexsize = 0;
        int datavectorsize = 0;
        int datasize = 0;
        if (usedataindex_) {
            // headerindexsize will be used to skip or not to skip the bytes that represent the index
            // so if the user specified useDataIndex(true), it means we do not skip the index byte
            // thus the headerindexsize is only the size of headersize_
            headerindexsize = headersize_;

            // we should expect how much the size of the data we will get
            // ultrasound_frd needs to be resized with the index byte
            datasize = indexsize_ + (sizeof(double) * datalength_);
            ultrasound_frd.resize(datasize / sizeof(double));
        }

        else {
            // if the user specified otherwise, it means we skip the index byte
            // thus the headerindexsize is the total of headersize_ + indexsize_
            headerindexsize = headersize_ + indexsize_;

            // ultrasound_frd needs to be resized with the index byte
            datasize = 0 + (sizeof(double) * datalength_);
            ultrasound_frd.resize(datasize / sizeof(double));
        }

        // temporary buffer that we will use for store binary string from socket
        // the A-mode ultrasound machine always send full data (header+index+data)
        // so we need to accomodate the full data
        int receivebuffersize = (sizeof(double) * datalength_) + headersize_ + indexsize_;
        char* receivebuffer = new char[receivebuffersize];
        int bytereceived = 0;

        count_streameddata_ = 0;
        count_recordeddata_ = 0;
        timestamp = rtb::getTime();

        myprintFormat(AModeUSConnection::MESSAGE_RUNNING, "Start streaming A-mode US Depth Data.");
        // main loop to receive the data,
        // we will do this until there is an error or one of the system is stop
        do {
            bytereceived = receiveData(&ultrasound_frd, receivebuffer, receivebuffersize, datasize, headerindexsize);
        } while (bytereceived > 0 && !synch::getStop() && !userquit_);
        //} while (bytereceived > 0 && !userquit_);
    }

    double timestamp2 = rtb::getTime();
    char strbuffer[100];
    sprintf(strbuffer, "Stop streaming. Streamed: %d, Recorded: %d, time elapsed: %.4f, data rate: %.4fs", count_streameddata_, count_recordeddata_, (timestamp2 - timestamp), (timestamp2 - timestamp) / count_streameddata_);
    myprintFormat(AModeUSConnection::MESSAGE_OK, strbuffer);
    //std::cout << "[AMode]\t[>>] " << timestamp2 << " - " << timestamp << " = " << timestamp2 - timestamp << " (" << (timestamp2 - timestamp) / countdata_ << ")\n";

    // disconnect the socket, we want everything is clean after this program is stopped
    // https://docs.microsoft.com/en-us/windows/win32/winsock/disconnecting-the-client
    iResult = shutdown(ConnectSocket_, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        char strbuffer[100];
        sprintf(strbuffer, "shutdown failed: %ld", WSAGetLastError());
        myprintFormat(AModeUSConnection::MESSAGE_WARNING, strbuffer);
        closesocket(ConnectSocket_);
        WSACleanup();
    }
    closesocket(ConnectSocket_);
    WSACleanup();

    // close the filestream
    if (ofs_.is_open()) {
        ofs_.close();
    }
}

void AModeUSConnection::myprintFormat(AModeUSConnection::enumMessageType messagetype, std::string message)
{
    std::string header = "[A-Mode]";
    std::string icon = "";

    if (messagetype == AModeUSConnection::MESSAGE_OK) icon = "[OK]";
    else if (messagetype == AModeUSConnection::MESSAGE_WARNING) icon = "[!!]";
    else if (messagetype == AModeUSConnection::MESSAGE_RUNNING) icon = "[..]";
    else printf(" ?? ");

    printf("%s\t%s %s\n", header.c_str(), icon.c_str(), message.c_str());
}