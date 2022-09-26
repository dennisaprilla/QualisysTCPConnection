#include "QualisysConnection.h"



QualisysConnection::QualisysConnection()
{
    this->connectTCP();
    this->readMarkerSettings();
}

QualisysConnection::QualisysConnection(std::string ip, unsigned short port)
{
    ip_ = ip;
    port_ = port;
    this->connectTCP();
    this->readMarkerSettings();
}

QualisysConnection::~QualisysConnection()
{
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
            printf("[!!] rtProtocol.Connect: %s\n\n", poRTProtocol_.GetErrorString());
            sleep(1);
            return -1;
        }
    }
    // if there is nothing wrong, return 0; 
    printf("[OK] Connected to the Qualisys Motion Tracking system specified at ip: %s, port: %d\n", (char*)ip_.data(), port_);
    return 0;

}


int QualisysConnection::setControlGUIRecord(std::string password)
{
    // trying to take control Qualisys GUI
    if ( poRTProtocol_.TakeControl(password.c_str()) )
    {   
        // take controll is successful
        printf("[OK] Successfully gain control of Qualisys GUI.\n");
        // let's set a flag to inform the program that the user can control GUI record now
        controlGUIrecord_ = true;
        return 0;
    }
    else
    {
        // take controll is unsuccessful
        printf("[!!] Failed to take control over QTM. %s\n", poRTProtocol_.GetErrorString());
        // let's set a flag to inform the program that the user can not control GUI record
        controlGUIrecord_ = false;
        return -1;
    }
}


int QualisysConnection::readMarkerSettings() 
{
    // read the rigid body settings
    bool bDataAvailable;
    if (!poRTProtocol_.Read6DOFSettings(bDataAvailable))
    {
        printf("[!!] rtProtocol.Read6DOFSettings: %s\n\n", poRTProtocol_.GetErrorString());
        synch::setStop(true);
        return -1;
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

    printf("[OK] Recieved 6DoF settings from Qualisys.\n");
    return 0;
}


int QualisysConnection::receiveData()
{
    // (!) This block will be ignored if the user specified to control QTM GUI from CMD
    // i need to continuously listening if there is any event (such as start capture or stop capture from QTM GUI).
    // don't start recording if there is no signal from the QTM GUI yet.
    if (!controlGUIrecord_)
    {
        // variable to capture packet event from qualisys
        CRTPacket::EEvent ePacketEvent;
        
        if (poRTProtocol_.GetState(ePacketEvent, true, 1)) //1ms
        {
            if (ePacketEvent == CRTPacket::EEvent::EventCaptureStarted && !userstart_)
            {
                std::cout << "[>>] Start capturing Qualisys (controlled manually from GUI)." << std::endl;
                // If qualisys recording started we also start the recording of US image
                synch::start();
                userstart_ = true;
            }

            //If qualisys recording stopped we also stop the software
            if (ePacketEvent == CRTPacket::EEvent::EventCaptureStopped && userstart_)
            {
                // If qualisys recording stopped we also start the recording of other devices
                std::cout << "[>>] Qualisys recording stopped." << std::endl;
                synch::setStop(true);
                userstart_ = false;
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
                std::cout << "[!!] Error when streaming frames: " << poRTProtocol_.GetRTPacket()->GetErrorString() << std::endl;
                synch::setStop(true);
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
                std::cout << "[!!] Unknown CRTPacket case" << std::endl;
                break;
            }

        // if receiving data is not successful;
        } else {
            // return -1;

        } // end if .Receive();

    } // end if (userstart_);

    // check if user presed a key
    userquit_ = this->checkKeyPressed();

    return 0;
}


void QualisysConnection::operator()()
{
    // if user specified record, create new log using QualisysLogger (inherited from OpenSimFileLogger)
    if (record_)
    {
        logger_ = new QualisysLogger(recordDirectory_);
        logger_->addLog(Logger::LogID::RigidBody, rigidbodyName_);
    }

    // if user specified to control QTM GUI from CMD to record, let's command startcapture,
    // if not, skip this part, and in the receiveData() there will be listener to start event from GUI
    if (controlGUIrecord_)
    {
        // try to command to start capture
        if (poRTProtocol_.StartCapture()) {
            printf("[>>] Start capturing Qualisys (controlled from CMD).\n");
            synch::start();
            userstart_ = true;

        // if the command fails
        } else {
            printf("[!!] Start capturing failed. (%s)\n", poRTProtocol_.GetErrorString());
            synch::setStop(true);
            userstart_ = false;
        }
    }

    // a flag if there is a condition that terminates the connection
    int streamingstatus=-1;
    // as long as there is no stopping signal from every other device, keep receiving data 
    while (!synch::getStop() && !userquit_) {
        // receive the data
        streamingstatus = this->receiveData();
    }


    // streaming not going well
    if (streamingstatus==-1) {
        printf("[!!] There is something wrong with the streaming data.\n");
        synch::setStop(true);
    }

    // streaming goes well until the user pressed ESC
    if (streamingstatus==0) {
        // if user specified to controling QTM GUI from CMD, let's command to stop and save
        if (controlGUIrecord_) {

            // first, let's command QTM to stop
            if (poRTProtocol_.StopCapture()) {
                
                // stop every other devices
                synch::setStop(true);
                printf("[OK] Successfully stopped QTM recording.");
                // now, after QTM stopped, let's command QTM to save the data
                std::string filename = "QTMdata";

                if (poRTProtocol_.SaveCapture(filename.c_str(), true, nullptr, 0)) {
                    // if successfull to save
                    printf("[OK] Successfully saved QTM recording.\n");
                }
                else {
                    // if fail to save
                    printf("[!!] Something wrong happened when saving QTM recording.\n");
                }

            } else {
                // if fail to stop
                printf("[!!] Something wrong happened when stopping QTM recording. Saving data failed.\n");
                synch::setStop(true);
            }

        // if user didn't specify controlling GUI from CMD, just quit
        } else {
            printf("[>>] Streaming finished. Bye-bye!\n");
            synch::setStop(true);
        }
    }
}
