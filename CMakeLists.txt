project(HtProtocol C CXX)
cmake_minimum_required( VERSION 3.4 )
# set(CMAKE_CXX_STANDARD 11)
add_definitions(-std=c++11)
aux_source_directory(src/${SUB_DIR} PROTOCOL_SRC)
file(GLOB allCopyFiles "src/*.h")

add_library(HtProtocol_shared SHARED ${PROTOCOL_SRC})
target_link_libraries(HtProtocol_shared ${LIB})


link_libraries(
    # HtProtocol
    pthread 
)
add_subdirectory(tests)