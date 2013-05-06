cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED QtNetwork)
#TODO: Create a find_package for libqjson
include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set(qtRosBridge_SRCS websocket.cpp qtRosBridge.cpp)
qt4_automoc(${qtRosBridge_SRCS})
ADD_LIBRARY(qtRosBridge STATIC ${qtRosBridge_SRCS})
target_link_libraries(qtRosBridge ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY} qjson)
