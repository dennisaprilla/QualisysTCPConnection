# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("QualisysTCPConnect")

# Boost library
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(BOOST_INCLUDEDIR "C:\\boost_1_80_0\\boost")
set(BOOST_LIBRARYDIR "C:\\boost_1_80_0\\stage\\lib")
set(BOOST_ROOT "C:\\boost_1_80_0")
find_package(Boost 1.80.0 COMPONENTS REQUIRED system filesystem)
include_directories(${Boost_INCLUDE_DIRS} )

# Opencv library
# Don't forget to put the C:\opencv\build\x64\vc15\bin to PATH
set(OpenCV_DIR "C:\\opencv\\build")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
link_directories( ${OpenCV_LIB_DIR} )

# Eigen library (only headers, no instalation)
include_directories("C:\\eigen-3.4.0")

include_directories(
	"include"
)

# TCLAP library (only headers, no installation)
include_directories("C:\\tclap-1.4.0-rc1\\include")

# Include sub-projects.
add_subdirectory ("external/logger")
add_subdirectory ("external/synch")
add_subdirectory ("external/qualisys_cpp_sdk")
add_subdirectory ("src")


