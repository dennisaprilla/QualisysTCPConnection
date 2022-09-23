/*
 * Copyright (c) 2016, <copyright holder> <email>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "OpenSimFileLogger.h"

OpenSimFileLogger::OpenSimFileLogger(const std::string& recordDirectory) :
	_subjectModelGiven(false), _recordDirectory(recordDirectory), _cptMarker(0), _cptMarkerFilter(0), threadStop_(false)
{
	// Create the directory for the recorded file
	std::stringstream ss;
	//ss << "./";
	ss << _recordDirectory;
	boost::filesystem::path dir(ss.str().c_str());

	if (!boost::filesystem::exists(dir))
		if (!boost::filesystem::create_directories(dir))
			std::cout << "ERROR in creating directory: " << ss.str() << std::endl;

	saveThread_ = new std::thread(&OpenSimFileLogger::threadFunc, this);
}


OpenSimFileLogger::~OpenSimFileLogger()
{

	for (std::map<Logger::LogID, std::ofstream*>::iterator it = _mapLogIDToFile.begin(); it != _mapLogIDToFile.end(); it++)
		delete it->second;

	for (std::map<std::string, std::ofstream*>::iterator it = _mapMANametoFile.begin(); it != _mapMANametoFile.end(); it++)
		delete it->second;
}


void OpenSimFileLogger::threadFunc()
{

	mtxEnd_.lock();
	bool threadStop = threadStop_;
	mtxEnd_.unlock();
	while (!threadStop || loggerStructDeque_.size() > 0)
	{
		bool empty = true;
		Logger::loggerStruct temp;
		mtxdata_.lock();
		if (loggerStructDeque_.size() > 0)
		{
			temp = loggerStructDeque_.front();
			loggerStructDeque_.pop_front();
			empty = false;
		}
		mtxdata_.unlock();
		if (!empty)
		{
			*temp.file << std::setprecision(15) << temp.time << "\t";

			for (std::vector<double>::const_iterator it = temp.data.begin(); it != temp.data.end(); it++)
				*temp.file << std::setprecision(15) << *it << "\t";

			*temp.file << std::endl;
		}
		else
		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(300ms);//Waiting for more data
		}


		mtxEnd_.lock();
		threadStop = threadStop_;
		mtxEnd_.unlock();

	}
	std::cout << "logger Size" << loggerStructDeque_.size() << std::endl;
}


void OpenSimFileLogger::addLog(Logger::LogID logID, const std::vector<std::string>& ColumnName)
{
	using namespace Logger;
	std::string filename;
	std::stringstream ss;

	switch (logID)
	{
	case Marker:
		filename = "/marker.trc";
		_mapLogIDToNumerOfRow[Marker] = 0;
		break;

	case MarkerQualisysTime:
		filename = "/markerQualisysTime.trc";
		_mapLogIDToNumerOfRow[MarkerQualisysTime] = 0;
		break;

	case ForcePlate:
		filename = "/forcePlate.mot";
		_mapLogIDToNumerOfRow[ForcePlate] = 0;
		break;

	case ForcePlateFilter:
		filename = "/forcePlateFilt.mot";
		_mapLogIDToNumerOfRow[ForcePlateFilter] = 0;
		break;


	}

	ss << "./";
	ss << _recordDirectory;
	ss << filename;
	_mapLogIDToFile[logID] = new std::ofstream(ss.str().c_str());
	std::ofstream* file = _mapLogIDToFile[logID];

	if (!(file->is_open()))
		std::cout << "ERROR: " + ss.str() + " cannot be opened!" << std::endl;

	HeaderFile headerFile;
	headerFile.setNumberOfRow(0);
	headerFile.setNumberOfColumn(ColumnName.size() + 1);// +1 for time
	headerFile.setNameOfColumn(ColumnName);
	headerFile.setInDegrees(false);

	switch (logID)
	{


	case ForcePlate:
		headerFile.writeFile(*file, ss.str(), "ForcePlate");
		break;

	case ForcePlateFilter:
		headerFile.writeFile(*file, ss.str(), "ForcePlateFilter");
		break;

	case Marker:
		_mapLogIDToNumerOfRow[Marker] = 0;
		*file << "PathFileType	4	(X/Y/Z)	marker.trc" << std::endl;
		markerHearder(*file, ColumnName, 0);
		columnMarkerNames_ = ColumnName;
		break;

	case MarkerQualisysTime:
		_mapLogIDToNumerOfRow[MarkerQualisysTime] = 0;
		*file << "PathFileType	4	(X/Y/Z)	MarkerQualisysTime.trc" << std::endl;
		markerHearder(*file, ColumnName, 0);
		columnMarkerNames_ = ColumnName;
		break;
	}

}


void OpenSimFileLogger::addLog(Logger::LogID logID)
{

}


void OpenSimFileLogger::markerHearder(std::ofstream& file, const std::vector<std::string>& ColumnName, const unsigned int& numbersOfFrames)
{
	file << "DataRate\tCameraRate\tNumFrames\tNumMarkers\tUnits\tOrigDataRate\tOrigDataStartFrame\tOrigNumFrames" << std::endl;
	file << "128\t128\t" << numbersOfFrames << "\t" << ColumnName.size() << "\tm\t128\t1\t" << numbersOfFrames << std::endl;
	file << "Frame#\tTime\t";

	for (std::vector<std::string>::const_iterator it = ColumnName.begin(); it != ColumnName.end(); it++)
		file << *it << "\t";

	file << std::endl;
	file << "\t\t";

	for (int cpt = 1; cpt < ColumnName.size() + 1; cpt++)
	{
		file << "X" << cpt << "\t";
		file << "Y" << cpt << "\t";
		file << "Z" << cpt << "\t";
	}

	file << std::endl;
	file << std::endl;
}


void OpenSimFileLogger::addMa(const std::string& maName, const std::vector<std::string>& ColumnName)
{
	using namespace Logger;
	std::stringstream ss;
	HeaderFile headerFile;
	ss << "./";
	ss << _recordDirectory;
	ss << "/ma_" << maName << ".sto";
	_mapMANametoFile[maName] = new std::ofstream(ss.str().c_str());
	std::ofstream* file = _mapMANametoFile[maName];
	_mapMANametoNumerOfRow[maName] = 0;
	headerFile.setNumberOfRow(0);
	headerFile.setNumberOfColumn(ColumnName.size() + 1); // +1 for time
	headerFile.setNameOfColumn(ColumnName);
	headerFile.setInDegrees(false);
	headerFile.writeFile(*file, "ma_" + maName + ".sto", "ma_" + maName);
}


void OpenSimFileLogger::log(Logger::LogID logID, const double& time)
{

}


void OpenSimFileLogger::log(Logger::LogID logID, const double& time, const std::vector<double>& data)
{
	using namespace Logger;
	std::ofstream* file = _mapLogIDToFile.at(logID);

	switch (logID)
	{


	case ForcePlate:
		_mapLogIDToNumerOfRow[ForcePlate]++;
		fillData(*file, time, data);
		break;

	case ForcePlateFilter:
		_mapLogIDToNumerOfRow[ForcePlateFilter]++;
		fillData(*file, time, data);
		break;



	case Marker:
		_mapLogIDToNumerOfRow[Marker]++;
		*file << _cptMarker << "\t" << std::setprecision(15) << time << "\t";

		for (std::vector<double>::const_iterator it = data.begin(); it != data.end(); it++)
			*file << std::setprecision(15) << *it << "\t";

		*file << std::endl;
		_cptMarker++;
		break;

	case MarkerQualisysTime:
		_mapLogIDToNumerOfRow[MarkerQualisysTime]++;
		*file << _cptMarkerFilter << "\t" << std::setprecision(15) << time << "\t";

		for (std::vector<double>::const_iterator it = data.begin(); it != data.end(); it++)
			*file << std::setprecision(15) << *it << "\t";

		*file << std::endl;
		_cptMarkerFilter++;
		break;
	}
}


void OpenSimFileLogger::log(Logger::LogID logID, const double& time, const double& data)
{
	using namespace Logger;
	// 	std::cout << time << std::endl;
	std::ofstream* file = _mapLogIDToFile.at(logID);

	_mapLogIDToNumerOfRow[logID]++;

	*file << std::setprecision(15) << time << "\t" << data << std::endl;
}


void OpenSimFileLogger::log(Logger::LogID logID, const double& time, const std::vector<bool>& data)
{

}


void OpenSimFileLogger::fillData(std::ofstream& file, const double& time, const std::vector<double>& data)
{
	Logger::loggerStruct temp;
	temp.file = &file;
	temp.time = time;
	temp.data = data;
	mtxdata_.lock();
	loggerStructDeque_.push_back(temp);
	mtxdata_.unlock();
}


void OpenSimFileLogger::fillData(std::ofstream& file, const double& time, const std::vector<bool>& data)
{
	file << std::setprecision(15) << time << "\t";

	for (std::vector<bool>::const_iterator it = data.begin(); it != data.end(); it++)
		file << std::setprecision(15) << *it << "\t";

	file << std::endl;
}


void OpenSimFileLogger::logMa(const std::vector<std::string>& maNameList, const double& time, const std::vector<std::vector<double> >& data)
{
	for (std::vector<std::string>::const_iterator it = maNameList.begin(); it != maNameList.end(); it++)
	{
		int cpt = std::distance<std::vector<std::string>::const_iterator>(maNameList.begin(), it);
		logMa(*it, time, data.at(cpt));
	}
}


void OpenSimFileLogger::logMa(const std::string& maName, const double& time, const std::vector<double>& data)
{
	std::ofstream* file = _mapMANametoFile.at(maName);
	_mapMANametoNumerOfRow[maName]++;
	fillData(*file, time, data);
}


void OpenSimFileLogger::stop()
{

	std::cout << "OpenSimFileLogger::stop() " << this << std::endl;
	using namespace Logger;
	mtxEnd_.lock();
	threadStop_ = true;
	mtxEnd_.unlock();
	saveThread_->join();
	delete saveThread_;

	for (std::map<Logger::LogID, std::ofstream* >::iterator it = _mapLogIDToFile.begin(); it != _mapLogIDToFile.end(); it++)
	{

		std::string header;
		std::stringstream ss, ssCopy;
		ss << "./";
		ss << _recordDirectory;
		ssCopy << "./";
		ssCopy << _recordDirectory;

		switch (it->first)
		{


		case ForcePlate:
			header = "forcePlate";
			ss << "/forcePlate.mot";
			ssCopy << "/forcePlate_copy.mot";
			break;

		case ForcePlateFilter:
			header = "forcePlateFilter";
			ss << "/forcePlateFilt.mot";
			ssCopy << "/forcePlateFilt_copy.mot";
			break;



		case Marker:
			ss << "/marker.trc";
			ssCopy << "/marker_copy.trc";
			break;

		case MarkerQualisysTime:
			ss << "/MarkerQualisysTime.trc";
			ssCopy << "/MarkerQualisysTime_copy.trc";
			break;
		}
		it->second->close();
#ifdef WIN32
		if (std::rename(ss.str().c_str(), ssCopy.str().c_str()) == -1)
		{
			boost::filesystem::path full_path(boost::filesystem::current_path());
			std::cout << "Current path is : " << full_path << std::endl;
			std::cout << ss.str().c_str() << std::endl;
			std::cout << " Error: " << strerror(errno) << std::endl;
		}
#else
		std::rename(ss.str().c_str(), ssCopy.str().c_str());
#endif

		std::ifstream infile(ssCopy.str().c_str());
		std::ofstream outfile(ss.str().c_str());

		if (it->first != MarkerQualisysTime && it->first != Marker)
		{
			HeaderFile headerFile;
			headerFile.readFile(infile, ssCopy.str().c_str());
			headerFile.setNumberOfRow(_mapLogIDToNumerOfRow[it->first] - 1);
			headerFile.writeFile(outfile, ss.str(), header);
		}
		else if (it->first == MarkerQualisysTime || it->first == Marker)
		{
			outfile << "PathFileType\t4\t(X/Y/Z)\tmarker.trc" << std::endl;
			markerHearder(outfile, columnMarkerNames_, _mapLogIDToNumerOfRow[it->first]);
			std::string line;
			std::getline(infile, line, '\n');
			std::getline(infile, line, '\n');
			std::getline(infile, line, '\n');
			std::getline(infile, line, '\n');
			std::getline(infile, line, '\n');
			std::getline(infile, line, '\n');
		}

		outfile << infile.rdbuf();
		outfile.close();
		infile.close();
		std::remove(ssCopy.str().c_str());
	}


	for (std::map<std::string, std::ofstream*>::iterator it = _mapMANametoFile.begin(); it != _mapMANametoFile.end(); it++)
	{
		std::stringstream ss, ssCopy;
		ss << "./";
		ss << _recordDirectory;
		ss << "/ma_" << it->first << ".sto";
		ssCopy << "./";
		ssCopy << _recordDirectory;
		ssCopy << "/ma_" << it->first << "_copy.sto";
		it->second->close();
		std::rename(ss.str().c_str(), ssCopy.str().c_str());
		std::ifstream infile(ssCopy.str().c_str());
		std::ofstream outfile(ss.str().c_str());
		HeaderFile headerFile;
		headerFile.readFile(infile, ssCopy.str().c_str());
		headerFile.setNumberOfRow(_mapMANametoNumerOfRow[it->first]);
		headerFile.writeFile(outfile, ss.str(), it->first);
		outfile << infile.rdbuf();
		outfile.close();
		infile.close();
		std::remove(ssCopy.str().c_str());
	}
}
