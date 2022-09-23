add_library(Synch
	"Synch.cpp"
	"getTime.h"
)

target_include_directories(Synch PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})