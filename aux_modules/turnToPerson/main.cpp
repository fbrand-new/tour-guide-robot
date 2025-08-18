/*
 * Copyright (C) 2006-2025 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#include "turnToPerson.h"

int main(int argc, char *argv[])
{
    // Initialize yarp network
    yarp::os::Network yarp;

    // Prepare and configure the resource finder
    yarp::os::ResourceFinder rf;
    rf.configure(argc, argv);

    TurnToPerson module("turnToPerson");

    if (!module.runModule(rf))
    {
        yError() << "Error module turnToPerson did not start";
        return -1;
    }

    return 0;
}