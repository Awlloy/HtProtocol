project(HtProtocol C CXX)
cmake_minimum_required( VERSION 3.4 )
# set(CMAKE_CXX_STANDARD 11)
add_definitions(-std=c++11)
aux_source_directory(src/${SUB_DIR} PROTOCOL_SRC)
file(GLOB allCopyFiles "src/*.h")

# file(GLOB files  RELATIVE ${PROJECT_SOURCE_DIR} src/*.h)
# file(GLOB files  RELATIVE ${PROJECT_SOURCE_DIR} src/*.cpp)
# string(REPALCE ${PROJECT_SOURCE_DIR}/"" relpath ${file_path}) 
# message(${files})
include_directories(
    # ${FFMPEG_DIR}/include
    .
)
link_directories(
    # ${FFMPEG_DIR}/lib
    
)
set(LIB 
   
    # ${PROJECT_SOURCE_DIR}/libprotocol.a
)

# include(
#     ./HtProtocol.h
#     ./val_data.h
# )

# set( PROTOCOL_SRC 
#     ./HtProtocol.cpp 
#     ./val_data.cpp
# )
add_library(HtProtocol_shared SHARED ${PROTOCOL_SRC})
target_link_libraries(HtProtocol_shared ${LIB})


link_libraries(
    # HtProtocol
    pthread 
)
add_subdirectory(tests)
# add_executable(main ./main.cpp ${PROTOCOL_SRC} )
# add_executable(client ./client.cpp  ${PROTOCOL_SRC})
# add_executable(sender ./sender.cpp  ${PROTOCOL_SRC})

# target_link_libraries( main )
# target_link_libraries( client )
# target_link_libraries( sender )


# install(TARGETS protocol LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/build/lib)
# install(TARGETS img LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/build/lib)
# install(TARGETS ${LIB} LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/build/lib)

# install(FILES ${INCLUDE} DESTINATION ${PROJECT_SOURCE_DIR}/build/include)

# add_library(protocol STATIC ${SRC})

# target_link_libraries( test_jpeg)
# target_link_libraries( test_jpeg x264 pthread)
# target_link_libraries( test_jpeg  ${OpenCV_LIBS}  ${JPEG_Cflags} x264 pthread)
