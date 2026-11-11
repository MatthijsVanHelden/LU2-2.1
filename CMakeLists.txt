cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(Prototype CXX)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(tcp_a tcp_a.cpp)
add_executable(tcp_b tcp_b.cpp)
add_executable(storing storing.cpp)
add_executable(hb_a hb_a.cpp)
add_executable(hb_b hb_b.cpp)
target_link_libraries(tcp_a ws2_32)
target_link_libraries(tcp_b ws2_32)
target_link_libraries(storing ws2_32)
target_link_libraries(hb_a ws2_32)
target_link_libraries(hb_b ws2_32)

find_package(PahoMqttCpp CONFIG REQUIRED)
add_compile_definitions(PAHO_MQTTPP_IMPORTS)
add_executable(mqtt_a mqtt_a.cpp)
add_executable(mqtt_b mqtt_b.cpp)
target_link_libraries(mqtt_a PahoMqttCpp::paho-mqttpp3)
target_link_libraries(mqtt_b PahoMqttCpp::paho-mqttpp3)
