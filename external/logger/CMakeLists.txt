add_library(LoggerLib
	"OpenSimFileLogger.cpp"
	"HeaderFile.cpp"
)

target_include_directories(LoggerLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})