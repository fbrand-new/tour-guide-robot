/*
 * Copyright (C) 2006-2025 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#ifndef TURN_TO_PERSON_H
#define TURN_TO_PERSON_H

#include <yarp/dev/PolyDriver.h>
#include <yarp/os/Property.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/Network.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/RpcServer.h>
#include <yarp/os/Vocab.h>
#include <yarp/os/LogComponent.h>
#include <yarp/os/PortReader.h>
#include <yarp/os/ConnectionReader.h>
#include <iostream>
#include <math.h>
#include <vector>

/**
 * @brief Structure to hold person keypoints information
 */
struct PersonKeypoints
{
    std::vector<std::string> keypoint_names;
    std::vector<double> u_coords;  // image u coordinates
    std::vector<double> v_coords;  // image v coordinates
    double centroid_u = 0.0;
    double centroid_v = 0.0;
    bool valid = false;
};

/**
 * @brief YARP module that receives person keypoints and turns the robot towards the person
 */
class TurnToPerson : public yarp::os::RFModule, public yarp::os::PortReader
{
public:
    TurnToPerson(std::string name);

    // RFModule members
    bool configure(yarp::os::ResourceFinder &rf) override;
    bool interruptModule() override;
    double getPeriod() override;
    bool updateModule() override;
    bool close() override;

private:
    // Module parameters
    double m_period;
    std::string m_moduleName;
    
    // Port names
    std::string m_keypointsInputName;
    std::string m_rpcPortName;
    
    // YARP ports
    yarp::os::BufferedPort<yarp::os::Bottle> m_keypointsInputPort;
    yarp::os::RpcServer m_rpcPort;
    yarp::os::BufferedPort<yarp::os::Bottle> m_velocityCommandPort;
    
    // baseControl2 connection
    std::string m_basecontrol_port;
    
    // Control parameters
    double m_image_center_u;      // Image center u coordinate
    double m_image_center_v;      // Image center v coordinate
    double m_angular_gain;        // Proportional gain for angular velocity
    double m_max_angular_vel;     // Maximum angular velocity (deg/s)
    double m_dead_zone;           // Dead zone around image center (pixels)
    double m_command_timeout;     // Timeout for velocity commands (s)
    bool m_active;                // Whether the module is actively controlling
    
    // Private methods
    /**
     * @brief Parse keypoints from input bottle and calculate centroid
     * @param keypointsBottle Input bottle containing keypoints data
     * @param person Output person keypoints structure
     * @return true if parsing successful and person detected
     */
    bool parseKeypoints(const yarp::os::Bottle* keypointsBottle, PersonKeypoints& person);
    
    /**
     * @brief Calculate centroid from valid keypoints
     * @param person Person keypoints structure
     * @return true if centroid calculated successfully
     */
    bool calculateCentroid(PersonKeypoints& person);
    
    /**
     * @brief Calculate angular velocity to turn towards person centroid
     * @param centroid_u Person centroid u coordinate
     * @return angular velocity in deg/s
     */
    double calculateAngularVelocity(double centroid_u);
    
    /**
     * @brief Send velocity command to baseControl2 via YARP port
     * @param angular_vel Angular velocity in deg/s
     * @return true if command sent successfully
     */
    bool sendVelocityCommand(double angular_vel);
    
    /**
     * @brief Send velocity command directly to baseControl2 port
     * @param angular_vel Angular velocity in deg/s
     * @return true if command sent successfully
     */
    bool sendDirectVelocityCommand(double angular_vel);
    
    /**
     * @brief Stop the robot by sending zero velocities
     * @return true if stop command sent successfully
     */
    bool stopRobot();
    
    /**
     * @brief Handle RPC commands
     * @param command Input command
     * @param reply Output reply
     * @return true if command handled successfully
     */
    bool respond(const yarp::os::Bottle& command, yarp::os::Bottle& reply) override;
    
    /**
     * @brief Read method for PortReader interface
     * @param reader Connection reader
     * @return true if read successful
     */
    bool read(yarp::os::ConnectionReader& reader) override;
};

#endif // TURN_TO_PERSON_H
