# CMakeList.txt : CMake project for TestQualisys7, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (${PROJECT_NAME} "main.cpp" )

# Add my own library
add_library(QualisysConnectionLib
	"QualisysConnection.cpp"
	"QualisysLogger.cpp"
	
)

# link the qualisys SDK to my own library
target_link_libraries(QualisysConnectionLib
	LoggerLib
	Synch
	qualisys_cpp_sdk
	${Boost_LIBRARIES}
)

# finally, link my own full library to this project
target_link_libraries(${PROJECT_NAME}
	QualisysConnectionLib
)

# TODO: Add tests and install targets if needed.
