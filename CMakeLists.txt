cmake_minimum_required(VERSION 2.8)

if(POLICY CMP0020)
	cmake_policy(SET CMP0020 NEW)
endif()

project(OpenThinning)

find_package(VTK REQUIRED)

file(GLOB CPP_FILES Source/*.cpp)
file(GLOB   H_FILES Source/*.h  )

include(${VTK_USE_FILE})

add_executable(OpenThinning ${CPP_FILES} ${H_FILES})

target_link_libraries(OpenThinning ${VTK_LIBRARIES})
