#include <mutex>
#include <condition_variable>
#include <iostream>
#include <cstdlib>

#ifndef SYNCH_H
#define SYNCH_H

/**
 * @brief Class for syncronization between framegrabber and mocap (and also stop the exec)
*/
class synch
{
public:
	/**
	 * @brief function for while loop in thread
	 * @return if we are stopping the exec 
	*/
	static bool getStop()
	{
		bool stop;
		stopMutex_.lock();
		stop = stop_;
		stopMutex_.unlock();
		return stop;
	}

	/**
	 * @brief if we want to stop the exec properly
	 * @param stop 
	*/
	static void setStop(bool stop)
	{
		stopMutex_.lock();
		stop_ = stop;
		stopMutex_.unlock();
		// if we were waiting to start if we don't start it will still wait and not stop
		if (stop == true)
		{
			std::cout << "stop" << std::endl;
			start();
		}
	}

	/**
	 * @brief Wait for the start of the recording of the mocap system
	*/
	static void waitStart()
	{
		std::unique_lock<std::mutex> lk(startMutex_);
		startCondVar_.wait(lk, [] {return start_; });
	}

	/**
	 * @brief start the recording of the video stream
	*/
	static void start()
	{
		startMutex_.lock();
		start_ = true;
		startMutex_.unlock();
		startCondVar_.notify_one();
	}

protected:
	static bool stop_;
	static std::mutex stopMutex_;

	static std::condition_variable startCondVar_;
	static std::mutex startMutex_;
	static bool start_;
};

#endif