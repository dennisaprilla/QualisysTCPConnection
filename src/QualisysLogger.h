#include "OpenSimFileLogger.h"


class QualisysLogger : public OpenSimFileLogger
{
public:

	// overiding constructor
	QualisysLogger(const std::string& recordDirectory) : OpenSimFileLogger(recordDirectory) {};

	// overriding log function
	void log(Logger::LogID logID, const double& timePC, const double& timeQ, const std::vector<double>& data);
	// overriding add-logging function
	void addLog(Logger::LogID logID, const std::vector<std::string>& ColumnName);
	// overriding header function
	void markerHearder(std::ofstream& FilePtr, const std::vector<std::string>& ColumnName, const unsigned int& numbersOfFrames);

	void helloguys();

private:

};