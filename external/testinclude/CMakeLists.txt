add_library(Adder
	"test1.cpp"
	"test2.cpp"
)

target_include_directories(Adder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})