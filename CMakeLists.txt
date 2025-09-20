cmake_minimum_required(VERSION 4.0)
project(Chess)

set(CMAKE_CXX_STANDARD 20)

add_executable(Chess main.cpp
        src/search.cpp
        src/search.h
        src/eval.cpp
        src/eval.h
        src/openingdb.cpp
        src/openingdb.h

        lib/surge/src/position.cpp
        lib/surge/src/position.h
        lib/surge/src/tables.cpp
        lib/surge/src/tables.h
        lib/surge/src/types.cpp
        lib/surge/src/types.h
)
