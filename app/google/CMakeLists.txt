################################################################################
#                                                                              #
# Copyright (C) 2020 Fondazione Istituto Italiano di Tecnologia (IIT)          #
# All Rights Reserved.                                                         #
#                                                                              #
################################################################################

set(appname google)

file(GLOB apps      ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.xml)

yarp_install(FILES ${apps}    DESTINATION ${TOUR-GUIDE-ROBOT_APPLICATIONS_INSTALL_DIR})
