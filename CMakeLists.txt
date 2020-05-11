set(CMAKE_INCLUDE_CURRENT_DIR ON)
cmake_minimum_required(VERSION 3.0.0)
project(client-server VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD_REQUIRED 17)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)
find_package(CURL REQUIRED)
include_directories(/usr/local/include ${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${CURL_INCLUDE_DIRS})
add_definitions(-DHAVE_CURL)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include_directories(submodules/tgbot-cpp/include)
add_subdirectory(submodules/tgbot-cpp)

set(SRV_HEADERS server/server.h
    server/rooms.h)
qt5_wrap_cpp(moc_sources_srv ${SRV_HEADERS})

add_executable(server_exec server/main.cpp ${moc_sources_srv})

find_package(Qt5 COMPONENTS Core Network REQUIRED)
target_link_libraries(server_exec
    Qt5::Core
    Qt5::Network
    TgBot
    ${CMAKE_THREAD_LIBS_INIT}
    ${OPENSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${CURL_LIBRARIES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
