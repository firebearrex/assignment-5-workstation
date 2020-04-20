cmake_minimum_required(VERSION 3.16)
project(assignment_5_workstation)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)
include_directories(assignment-5-threadpool-workstation/src)

add_executable(assignment_5_workstation
        src/file_util.c
        src/file_util.h
        src/http_methods.c
        src/http_methods.h
        src/http_request.c
        src/http_request.h
        src/http_server.c
        src/http_server.h
        src/http_util.c
        src/http_util.h
        src/media_util.c
        src/media_util.h
        src/network_util.c
        src/network_util.h
        src/properties.c
        src/properties.h
        src/string_util.c
        src/string_util.h
        src/time_util.c
        src/time_util.h
        README.md
        assignment-5-threadpool-workstation/src/thpool.c
        assignment-5-threadpool-workstation/src/thpool.h
#        example.c
        )
