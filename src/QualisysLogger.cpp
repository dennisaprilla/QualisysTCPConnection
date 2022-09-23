#include "QualisysLogger.h"

void QualisysLogger::helloguys(){
	printf("hello guys....");
};

void QualisysLogger::log(Logger::LogID logID, const double& timePC, const double& timeQ, const std::vector<double>& data)
{
	using namespace Logger;
	std::ofstream* file = _mapLogIDToFile.at(logID);

	switch (logID)
	{
		case Marker:
			_mapLogIDToNumerOfRow[Marker]++;
			*file << _cptMarker << "\t" << std::setprecision(15) << timePC << "\t";

			for (std::vector<double>::const_iterator it = data.begin(); it != data.end(); it++)
				*file << std::setprecision(15) << *it << "\t";

			*file << "\n";
			_cptMarker++;
			break;

		case MarkerQualisysTime:
			_mapLogIDToNumerOfRow[MarkerQualisysTime]++;
			*file << _cptMarkerFilter << "\t" << std::setprecision(15) << timePC << "\t";

			for (std::vector<double>::const_iterator it = data.begin(); it != data.end(); it++)
				*file << std::setprecision(15) << *it << "\t";

			*file << "\n";
			_cptMarkerFilter++;
			break;

		case RigidBody:
			_mapLogIDToNumerOfRow[RigidBody]++;
			*file << _cptMarker << "\t" << std::setprecision(15) << timePC << "\t" << timeQ << "\t";

			for (std::vector<double>::const_iterator it = data.begin(); it != data.end(); it++)
				*file << std::setprecision(15) << *it << "\t";

			*file << "\n";
			_cptMarker++;
			break;

		case RigidBodyQualisysTime:
			_mapLogIDToNumerOfRow[RigidBodyQualisysTime]++;
			*file << _cptMarkerFilter << "\t" << std::setprecision(15) << timePC << "\t" << timeQ << "\t";

			for (std::vector<double>::const_iterator it = data.begin(); it != data.end(); it++)
				*file << std::setprecision(15) << *it << "\t";

			*file << "\n";
			_cptMarkerFilter++;
			break;
	}
}

void QualisysLogger::addLog(Logger::LogID logID, const std::vector<std::string>& ColumnName)
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

		case RigidBody:
			filename = "/rigidbody.trc2";
			_mapLogIDToNumerOfRow[Marker] = 0;
			break;

		case RigidBodyQualisysTime:
			filename = "/rigidbodyQualisysTime.trc2";
			_mapLogIDToNumerOfRow[MarkerQualisysTime] = 0;
			break;
	}

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

		case RigidBody:
			_mapLogIDToNumerOfRow[RigidBody] = 0;
			*file << "PathFileType	4	(X/Y/Z)	rigidbody.trc2" << std::endl;
			markerHearder(*file, ColumnName, 0);
			columnMarkerNames_ = ColumnName;
			break;

		case RigidBodyQualisysTime:
			_mapLogIDToNumerOfRow[RigidBodyQualisysTime] = 0;
			*file << "PathFileType	4	(X/Y/Z)	rigidbodyQualisysTime.trc2" << std::endl;
			markerHearder(*file, ColumnName, 0);
			columnMarkerNames_ = ColumnName;
			break;
	}

}

void QualisysLogger::markerHearder(std::ofstream& file, const std::vector<std::string>& ColumnName, const unsigned int& numbersOfFrames)
{
	file << "DataRate\tCameraRate\tNumFrames\tNumMarkers\tUnits\tOrigDataRate\tOrigDataStartFrame\tOrigNumFrames" << std::endl;
	file << "128\t128\t" << numbersOfFrames << "\t" << ColumnName.size() << "\tm\t128\t1\t" << numbersOfFrames << std::endl;
	file << "Frame#\tTimePC\tTimeQ\t";

	for (std::vector<std::string>::const_iterator it = ColumnName.begin(); it != ColumnName.end(); it++)
		file << *it << "\t";

	file << "\n";
	file << "\t\t";

	for (int cpt = 1; cpt < ColumnName.size() + 1; cpt++)
	{
		file << "tx_" << cpt << "\t";
		file << "ty_" << cpt << "\t";
		file << "tz_" << cpt << "\t";
		file << "Qw_" << cpt << "\t";
		file << "Qx_" << cpt << "\t";
		file << "Qy_" << cpt << "\t";
		file << "Qz_" << cpt << "\t";
	}

	file << "\n";
	file << "\n";
}