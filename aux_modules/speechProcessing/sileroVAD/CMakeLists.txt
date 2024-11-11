################################################################################
#                                                                              #
# Copyright (C) 2020 Fondazione Istituto Italiano di Tecnologia (IIT)          #
# All Rights Reserved.                                                         #
#                                                                              #
################################################################################
project(sileroVAD)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(AUX_NAME sileroVAD)
add_executable(${AUX_NAME})
target_include_directories(${AUX_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    $<TARGET_PROPERTY:WakeMsgs,INTERFACE_INCLUDE_DIRECTORIES>
    /usr/local/src/robot/onnxruntime-linux-x64-1.20.0/include
)
target_sources(${AUX_NAME}
  PRIVATE
    main.cpp
    Detector.h
    Detector.cpp
    VoiceActivationDetectionModule.h
    VoiceActivationDetectionModule.cpp
)


target_sources(${AUX_NAME}
  PRIVATE
    ${STATECHARTS_FILES})

target_link_libraries(${AUX_NAME}
  PRIVATE
    ${YARP_LIBRARIES}
    WakeMsgs
    /usr/local/src/robot/onnxruntime-linux-x64-1.20.0/lib/libonnxruntime.so)


install(TARGETS ${AUX_NAME} DESTINATION bin)
