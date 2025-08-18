cmake_minimum_required(VERSION 3.12)

project(turnToPerson)

find_package(YARP 3.0.0 COMPONENTS os dev REQUIRED)

# Add the executable
add_executable(turnToPerson
    main.cpp
    turnToPerson.cpp
)

# Link against YARP libraries
target_link_libraries(turnToPerson
    ${YARP_LIBRARIES}
)

# Include directories
target_include_directories(turnToPerson PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install the executable
install(TARGETS turnToPerson
    DESTINATION bin
)
