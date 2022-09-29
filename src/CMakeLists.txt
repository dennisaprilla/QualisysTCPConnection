# CMakeList.txt : CMake project for TestQualisys7, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (${PROJECT_NAME} "main.cpp" )

# add my qualisys cpp files as a library: QualisysConnectionLib
add_library(QualisysConnectionLib
	"QualisysConnection.cpp"
	"QualisysLogger.cpp"
)

# link some other library that is required by my QualisysConnectionLib
target_link_libraries(QualisysConnectionLib
	LoggerLib
	Synch
	qualisys_cpp_sdk
	${Boost_LIBRARIES}
)

# add my amode cpp files as a library: AModeConnectionLib
add_library(AModeConnectionLib
	"AmodeUSConnection.cpp"
)

# link some other library that is required by my AModeConnectionLib
target_link_libraries(AModeConnectionLib
	Synch
	${OpenCV_LIBS}
	${Boost_LIBRARIES}
)

# finally, link my own full library to this project
target_link_libraries(${PROJECT_NAME}
	QualisysConnectionLib
	AModeConnectionLib
)

# TODO: Add tests and install targets if needed.
