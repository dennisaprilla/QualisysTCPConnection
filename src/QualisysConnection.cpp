#include "QualisysConnection.h"



QualisysConnection::QualisysConnection()
{
    statusQConnection_ = this->connectTCP();
    statusQMarker_     = this->readMarkerSettings();
}

QualisysConnection::QualisysConnection(std::string ip, unsigned short port)
{
    ip_ = ip;
    port_ = port;
    statusQConnection_ = this->connectTCP();
    statusQMarker_     = this->readMarkerSettings();
}

QualisysConnection::~QualisysConnection()
{
    //synch::start();
    //sleep(1);
    //synch::setStop(true);
}


int QualisysConnection::connectTCP()
{

    // if there is no connection yet to Qualisys...
    if (!poRTProtocol_.Connected())
    {
        // ...let's make connection
        if (!poRTProtocol_.Connect((char*)ip_.data(), port_, &udpPort, majorVersion, minorVersion, bigEndian))
        {
            // if fails, print the error message and return -1;
            myprintFormat(QualisysConnection::MESSAGE_WARNING, poRTProtocol_.GetErrorString());
            sleep(1);
            return 0;
        }
    }
    // if there is nothing wrong, return 0;
    char strbuffer[100];
    sprintf(strbuffer, "Connected to the Qualisys Motion Tracking system specified at ip: %s, port: %d", ip_.c_str(), port_);
    myprintFormat(QualisysConnection::MESSAGE_OK, strbuffer);

    return 1;
}


int QualisysConnection::readMarkerSettings() 
{
    // read the rigid body settings
    bool bDataAvailable;
    if (!poRTProtocol_.Read6DOFSettings(bDataAvailable))
    {
        char strbuffer[100];
        sprintf(strbuffer, "Read settings fails: %s.", poRTProtocol_.GetErrorString());
        myprintFormat(QualisysConnection::MESSAGE_WARNING, strbuffer);
        //synch::setStop(true);
        return 0;
    }

    // get the rigid body settings. well, tbh, here, we don't need to retrive all the settings data 
    // (color, position, scale, etc.). So, i will commented this part (until i think that i need it)
    // std::vector<CRTProtocol::SSettings6DOFBody> bodiesSettings;
    // poRTProtocol_.Get6DOFBodySettings(bodiesSettings);

    // get the number of rigid body detected by Qualisys
    int nBodies = poRTProtocol_.Get6DOFBodyCount();
    // get the data only if there is more than one rigid body detected.
    if (nBodies > 0)
    {
        // loop over all the rigid bodies detected
        for (int iBody = 0; iBody < nBodies; iBody++)
        {
            // retrieve each rigid body's name
            //const char* tmpName = poRTProtocol_.Get6DOFBodyName(iBody);
            std::string tmpstr(poRTProtocol_.Get6DOFBodyName(iBody));
            rigidbodyName_.push_back(tmpstr);

        }
    }

    myprintFormat(QualisysConnection::MESSAGE_OK, "Recieved 6DoF settings from Qualisys.");
    return 1;
}



int QualisysConnection::setControlGUICapture(std::string password)
{
    // trying to take control Qualisys GUI
    if (poRTProtocol_.TakeControl(password.c_str()))
    {
        // take controll is successful
        myprintFormat(QualisysConnection::MESSAGE_OK, "Successfully gain control of Qualisys GUI.");
        // let's set a flag to inform the program that the user can control GUI record now
        streamMode_ = QualisysConnection::STREAM_USING_COMMAND;
        return 1;
    }
    else
    {
        // take controll is unsuccessful
        std::string strtmp = "Failed to take control over QTM.";
        myprintFormat(QualisysConnection::MESSAGE_WARNING, strtmp);
        // let's set a flag to inform the program that the user can not control GUI record
        streamMode_ = QualisysConnection::STREAM_USING_MANUAL_BUTTON;
        return 0;
    }
}


int QualisysConnection::receiveData()
{
    // I need to continuously listening if there is any event (such as start capture or stop capture from QTM GUI).
    // don't start recording if there is no signal from the QTM GUI yet). This applied to both STREAM_USING_COMMAND
    // and STREAM_USING_MANUAL_BUTTON, because it takes time for QTM to start the capturing from the time this program
    // sent a command. This block will be ignored if the user specified STREAM_USING_NOTHING
    if ( (streamMode_ == QualisysConnection::STREAM_USING_MANUAL_BUTTON) || (streamMode_== QualisysConnection::STREAM_USING_COMMAND) )
    {
        // variable to capture packet event from qualisys
        CRTPacket::EEvent ePacketEvent;
        
        if (poRTProtocol_.GetState(ePacketEvent, true, 1)) //1ms
        {
            if (ePacketEvent == CRTPacket::EEvent::EventCaptureStarted && !userstart_)
            {
                myprintFormat(QualisysConnection::MESSAGE_RUNNING, "Start capturing Qualisys (capture commanded from QTM GUI).");
                // (!) START RECORDING FOR EVERY OTHER DEVICE
                synch::start();
                userstart_ = true;
            }

            //If qualisys recording stopped we also stop the software
            if (ePacketEvent == CRTPacket::EEvent::EventCaptureStopped && userstart_)
            {
                // If qualisys recording stopped we also start the recording of other devices
                myprintFormat(QualisysConnection::MESSAGE_WARNING, "Qualisys recording stopped.");
                synch::setStop(true);
                userstart_ = false;
                userquit_ = true;
            }
        }
    }


    // receving
    if (userstart_)
    {
        // variable to capture packettype (error/packetdata/end)
        CRTPacket::EPacketType ePacketType;

        // get a frame
        unsigned int nComponentType = CRTProtocol::cComponent6d;
        poRTProtocol_.GetCurrentFrame(nComponentType);
        // get a packet
        CRTPacket* rtPacket = poRTProtocol_.GetRTPacket();
        // Get the PC timeframe
        timeStamp_ = rtb::getTime();

        // check if receiving data is a success
        if (poRTProtocol_.Receive(ePacketType, true) == CNetwork::ResponseType::success)
        {
            // let's check type of data we got..
            switch (ePacketType)
            {
            
                // if there is a packet error, stop streaming, stop other device, and show errors
                case CRTPacket::PacketError:
                    myprintFormat(QualisysConnection::MESSAGE_WARNING, poRTProtocol_.GetRTPacket()->GetErrorString());
                    //synch::setStop(true);
                    break;

                    // if streaming is not running yet, it goes here
                    // or if there is no data at the moment
                case CRTPacket::PacketNoMoreData:
                    break;

                    // if we received data, let's do our bussiness
                case CRTPacket::PacketData:

                    float fX, fY, fZ;
                    float afRotMatrix[9];

                    // only concern if the packet arrived is in size with our expectation i.e. rigid body size
                    if (rtPacket->GetComponentSize(CRTPacket::Component6d))
                    {
                        // get how much number of 6DoF body we received
                        unsigned int nCount = rtPacket->Get6DOFBodyCount();
                        // we only concern if we really get some value
                        if (nCount > 0)
                        {
                            // clear the vector before inserting new data
                            rigidbodyData_.clear();

                            // get timestamp from Qualisys Packet
                            unsigned long long timestampQualisysInt = rtPacket->GetTimeStamp();
                            timeStampQualisys_ = double(timestampQualisysInt) / 1e6;


                            // loop for all rigid body detected
                            for (unsigned int i = 0; i < nCount; i++)
                            {
                                // get rigid body names
                                char* label = (char*)poRTProtocol_.Get6DOFBodyName(i);
                                // get the rigid body values
                                rtPacket->Get6DOFBody(i, fX, fY, fZ, afRotMatrix);

                                // using eigen to convert rotation matrix to quaternion
                                Eigen::Matrix3f RotM{ {afRotMatrix[0], afRotMatrix[1], afRotMatrix[2]},
                                                      {afRotMatrix[3], afRotMatrix[4], afRotMatrix[5]},
                                                      {afRotMatrix[6], afRotMatrix[7], afRotMatrix[8]} };
                                Eigen::Quaternionf q(RotM);

                                // print the values
                                //printf("%15s : %9.3f %9.3f %9.3f   %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f\n",
                                //    label, fX, fY, fZ, afRotMatrix[0], afRotMatrix[1], afRotMatrix[2], afRotMatrix[3],
                                //    afRotMatrix[4], afRotMatrix[5], afRotMatrix[6], afRotMatrix[7], afRotMatrix[8]);

                                // push the data to vector
                                rigidbodyData_.push_back(fX / 1000);
                                rigidbodyData_.push_back(fY / 1000);
                                rigidbodyData_.push_back(fZ / 1000);
                                rigidbodyData_.push_back(q.w());
                                rigidbodyData_.push_back(q.x());
                                rigidbodyData_.push_back(q.y());
                                rigidbodyData_.push_back(q.z());

                            }

                            // if the user decided to log, logging here
                            if (record_)
                            {
                                logger_->log(Logger::LogID::RigidBody, timeStamp_, timeStampQualisys_, rigidbodyData_);
                            }
                        }
                    }
                    break;

                // if there is some unknown package, ignore it
                default:
                    myprintFormat(QualisysConnection::MESSAGE_WARNING, "Unknown CRTPacket case");
                    break;
            }

        } else {
            // if receiving data is not successful;
            // return -1;

        } // end if .Receive();

    } // end if (userstart_);



    // check if user presed a key
    if (this->checkKeyPressed()) {

        // if user specified STREAM_USING_COMMAND, the program will send stop capture command and then wait and listening until
        // we recieve stop capture event from QTM GUI. In this period, we still streaming data.
        if (streamMode_ == QualisysConnection::STREAM_USING_COMMAND) {

            myprintFormat(QualisysConnection::MESSAGE_WARNING, "User pressed ESC. Sending Stop Capture command to QTM GUI");

            // check if we successfully command the QTM GUI to stop.
            // if successfull, it will continue the streaming until we recieve stop capture event from QTM GUI.
            // so, do nothing here, because it will be captured by the if block in the beginning of this function.
            if (poRTProtocol_.StopCapture()) {
                myprintFormat(QualisysConnection::MESSAGE_OK, "Stop capture command is successfully sent.");
            }
            // if fail, forget about saving the QTM file, there is something wrong with the stopping, quit immediately.
            // so, i just changed the streamMode_ to STREAM_USING_NOTHING;
            else {
                myprintFormat(QualisysConnection::MESSAGE_WARNING, "Something wrong happened when stopping QTM recording. Saving QTM file failed.");

                streamMode_ = QualisysConnection::STREAM_USING_NOTHING;
                userstart_ = false;
                userquit_ = true;
            }
        }

        // if the user specified STREAM_USING_NOTHING quit immediately
        else {
            myprintFormat(QualisysConnection::MESSAGE_WARNING, "User pressed ESC. Stop recording");
            userstart_ = false;
            userquit_ = true;
        }

        // PS. if the user specified STREAM_USING_MANUAL_BUTTON, ESQ will not stop the capturng until the user stop manually from the QTM GUI
    }

    return 1;
}


void QualisysConnection::operator()()
{

    // if user specified record, create new log using QualisysLogger (inherited from OpenSimFileLogger)
    if (record_)
    {
        logger_ = new QualisysLogger(recordDirectory_);
        logger_->addLog(Logger::LogID::RigidBody, rigidbodyName_);
    }

    // If user specified to control QTM GUI from CMD to record, it automatically uses STREAM_USING_COMMAND,
    // which will execute the start capture command. 
    if (streamMode_ == QualisysConnection::STREAM_USING_COMMAND)
    {
        // try to command to start capture
        if (poRTProtocol_.StartCapture()) {
            myprintFormat(QualisysConnection::MESSAGE_RUNNING, "Start capturing Qualisys (capture commanded from CMD, waiting for actual recording start)");

            // (!) START RECORDING FOR EVERY OTHER DEVICE
            // I changed my mind to start recording here, because when i test the command control to start
            // the recording, it seems there is a delay between command and the actual time the QTM start
            // capturing. So i will start other device to record until we got an event from QTM that it
            // actually starts recording (similar to STREAM_USING_MANUAL_BUTTON).
            // synch::start();

        // if the command fails
        } else {

            std::string strtmp = "Capture, commanded from CMD, is failed: ";
            strtmp += poRTProtocol_.GetErrorString();
            myprintFormat(QualisysConnection::MESSAGE_WARNING, strtmp);
            myprintFormat(QualisysConnection::MESSAGE_WARNING, "Stream mode changed from STREAM_USING_COMMAND to STREAM_USING_MANUAL_BUTTON. Start capture by pressing Capture Button in QTM GUI");

            streamMode_ = QualisysConnection::STREAM_USING_MANUAL_BUTTON;
            // release the control
            poRTProtocol_.ReleaseControl();
        }
    }

    // if the user not specified how the stream suppose to be, this is the deafult execution, it is just start
    // the streaming without anything (not command capture, nor listening capture button)
    if (streamMode_ == QualisysConnection::STREAM_USING_NOTHING)
    {
        // simulate delay that is happened with Qualisys Capture
        sleep(3000);
        // (!) START RECORDING FOR EVERY OTHER DEVICE
        myprintFormat(QualisysConnection::MESSAGE_RUNNING, "Start capturing Qualisys (no capture QTM)");
        synch::start();
        userstart_ = true;
    }


    // The main loop of receiving data is here ================================================================
 
    // a flag if there is a condition that terminates the connection
    int streamingstatus=0;
    // as long as there is no stopping signal from every other device, keep receiving data 
    while (!synch::getStop() && !userquit_) {
        // receive the data
        streamingstatus = this->receiveData();
    }
    // =========================================================================================================


    //// streaming not going well
    //if (streamingstatus==-1) {
    //    printf("[Qualisys]\t[!!] There is something wrong with the streaming data.\n");
    //    synch::setStop(true);
    //}

    // streaming goes well until the user pressed ESC
    if (streamingstatus) {
        // if user specified to controling QTM GUI from CMD, let's command to stop and save
        if (streamMode_ == QualisysConnection::STREAM_USING_COMMAND) {

            // The QTM is already stopped (look inside the receiveData() function).
            // Now, we need to save the QTM file.
            std::string filename = "QTMdata";
            if (poRTProtocol_.SaveCapture(filename.c_str(), true, nullptr, 0)) myprintFormat(QualisysConnection::MESSAGE_OK, "Successfully saved QTM recording.");
            else myprintFormat(QualisysConnection::MESSAGE_WARNING, "Something wrong happened when saving QTM recording.");

        // if user didn't specify controlling GUI from CMD, just quit
        } else {
            myprintFormat(QualisysConnection::MESSAGE_OK, "Streaming finished. Bye-bye!");
            synch::setStop(true);
        }
    }

}


void QualisysConnection::myprintFormat(QualisysConnection::enumMessageType messagetype, std::string message)
{
    std::string header = "[Qualisys]";
    std::string icon = "";

    if (messagetype == QualisysConnection::MESSAGE_OK) icon = "[OK]";
    else if (messagetype == QualisysConnection::MESSAGE_WARNING) icon = "[!!]";
    else if (messagetype == QualisysConnection::MESSAGE_RUNNING) icon = "[..]";
    else printf(" ?? ");

    printf("%s\t%s %s\n", header.c_str(), icon.c_str(), message.c_str());
}