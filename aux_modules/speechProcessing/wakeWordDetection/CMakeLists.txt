set(MODULE_NAME wakeWordDetection)
add_executable(${MODULE_NAME})

FetchContent_Declare(
    porcupine
    GIT_REPOSITORY https://github.com/Picovoice/porcupine.git
    GIT_TAG v3.0
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/porcupine
)
    
FetchContent_MakeAvailable(porcupine)

target_include_directories(${MODULE_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/porcupine/include
    $<TARGET_PROPERTY:WakeMsgs,INTERFACE_INCLUDE_DIRECTORIES>
)

target_sources(${MODULE_NAME}
  PRIVATE
    WakeWordModule.cpp
    WakeServer.cpp
    AudioCallback.cpp
    main.cpp
)

target_sources(${MODULE_NAME}
  PRIVATE
    ${STATECHARTS_FILES})

target_link_libraries(${MODULE_NAME}
  PRIVATE
    ${YARP_LIBRARIES}
    ${CMAKE_CURRENT_SOURCE_DIR}/porcupine/lib/linux/x86_64/libpv_porcupine.so
    WakeMsgs)
    
install(TARGETS ${MODULE_NAME} DESTINATION bin)