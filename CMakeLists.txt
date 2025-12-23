cmake_minimum_required(VERSION 3.10)
project(HedgeArbEA)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# DLL项目（供MQL4调用）
add_library(HedgeArbSharedMemory SHARED
    HedgeArbSharedMemory.cpp
    HedgeArbSharedMemory.h
)

# 总控程序
add_executable(HedgeArbControlProgram
    HedgeArbControlProgram.cpp
)

# 链接库
target_link_libraries(HedgeArbControlProgram HedgeArbSharedMemory)

# 输出目录
set_target_properties(HedgeArbSharedMemory PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

set_target_properties(HedgeArbControlProgram PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)


