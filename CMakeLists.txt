cmake_minimum_required(VERSION 3.13)
project(Chat)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} \
        -std=gnu++2a \
        -pedantic \
        -Wall \
        -Wextra\
         \
        -Wconversion \
        -Wenum-compare \
        -Wfloat-equal \
        -Woverloaded-virtual \
    "
    )

set(STATIC_BUILD FALSE)
if (CMAKE_BUILD_TYPE MATCHES Release)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    set(STATIC_BUILD TRUE)
endif ()

if (STATIC_BUILD)
    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc")
endif ()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Executable/bin)
set(ENV_ROOT "D:/Programms/ProgrammingEnvironment")


add_executable(${PROJECT_NAME}
    main.cpp

    src/Sockets/Sockets.cpp
    src/Sockets/Sockets.hpp

    src/Chat/Protocol.hpp
    src/Chat/Protocol.cpp

    src/Server/Server.cpp
    src/Server/Server.hpp

    src/Client/Client.cpp
    src/Client/Client.hpp

    )


target_link_libraries(${PROJECT_NAME} -lws2_32)
