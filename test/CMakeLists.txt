cmake_minimum_required(VERSION 3.10)
project(pdutest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(pdutest 
    pdutest.cpp 
    Arduino.cpp 
    ../src/pdu.cpp
)

target_include_directories(pdutest PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/../src
)

# Add include directories
target_include_directories(pdutest PRIVATE 
    ${CMAKE_SOURCE_DIR}/../include
)

# Add compiler definitions to simulate Arduino environment
target_compile_definitions(pdutest PRIVATE
    ARDUINO=100
    ESP8266
)

# Link against pthread for std::thread
find_package(Threads REQUIRED)
target_link_libraries(pdutest PRIVATE Threads::Threads)

# Link against Arduino libraries if needed
# target_link_libraries(pdutest PRIVATE arduino) 