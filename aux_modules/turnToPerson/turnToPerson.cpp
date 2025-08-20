/*
 * Copyright (C) 2006-2025 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#include "turnToPerson.h"
#include <yarp/os/LogComponent.h>
#include <yarp/os/Time.h>

YARP_LOG_COMPONENT(TURN_TO_PERSON, "navigation.turnToPerson")

TurnToPerson::TurnToPerson(std::string name) : 
    m_moduleName(name),
    m_period(0.1),
    m_image_center_u(320.0),
    m_image_center_v(240.0),
    m_angular_gain(0.1),
    m_max_angular_vel(30.0),
    m_dead_zone(20.0),
    m_command_timeout(1.0),
    m_min_angular_threshold(0.1),
    m_search_angular_vel(5.0),
    m_active(true)
{
}

bool TurnToPerson::configure(yarp::os::ResourceFinder &rf)
{
    // Read configuration parameters
    m_period = rf.check("period", yarp::os::Value(0.1)).asFloat64();
    m_moduleName = rf.check("name", yarp::os::Value("turnToPerson")).asString();
    
    // Image parameters
    m_image_center_u = rf.check("image_center_u", yarp::os::Value(320.0)).asFloat64();
    m_image_center_v = rf.check("image_center_v", yarp::os::Value(240.0)).asFloat64();
    
    // Control parameters
    m_angular_gain = rf.check("angular_gain", yarp::os::Value(0.1)).asFloat64();
    m_max_angular_vel = rf.check("max_angular_vel", yarp::os::Value(30.0)).asFloat64();
    m_dead_zone = rf.check("dead_zone", yarp::os::Value(20.0)).asFloat64();
    m_command_timeout = rf.check("command_timeout", yarp::os::Value(1.0)).asFloat64();
    
    // New parameters
    m_min_angular_threshold = rf.check("min_angular_threshold", yarp::os::Value(0.1)).asFloat64();
    m_search_angular_vel = rf.check("search_angular_vel", yarp::os::Value(5.0)).asFloat64();

    m_basecontrol_port = rf.check("basecontrol_port", yarp::os::Value("/baseControl/input/command/data:i")).asString();
    
    // Port names
    m_keypointsInputName = "/" + m_moduleName + "/keypoints:i";
    m_rpcPortName = "/" + m_moduleName + "/rpc";
    std::string velocityCommandPortName = "/" + m_moduleName + "/velocity:o";
    
    // Open ports
    if (!m_keypointsInputPort.open(m_keypointsInputName))
    {
        yCError(TURN_TO_PERSON) << "Cannot open keypoints input port" << m_keypointsInputName;
        return false;
    }
    
    if (!m_rpcPort.open(m_rpcPortName))
    {
        yCError(TURN_TO_PERSON) << "Cannot open RPC port" << m_rpcPortName;
        return false;
    }
    m_rpcPort.setReader(*this);
    
    // Open velocity command port for direct communication with baseControl2
    if (!m_velocityCommandPort.open(velocityCommandPortName))
    {
        yCError(TURN_TO_PERSON) << "Cannot open velocity command port" << velocityCommandPortName;
        return false;
    }
    
    // Try to connect to baseControl2
    if (!yarp::os::Network::connect(velocityCommandPortName, m_basecontrol_port))
    {
        yCWarning(TURN_TO_PERSON) << "Cannot connect to baseControl2 port" << m_basecontrol_port << ". Will try later.";
    }
    else
    {
        yCInfo(TURN_TO_PERSON) << "Connected to baseControl2 port" << m_basecontrol_port;
    }
    
    yCInfo(TURN_TO_PERSON) << "Module configured successfully";
    yCInfo(TURN_TO_PERSON) << "Image center: (" << m_image_center_u << ", " << m_image_center_v << ")";
    yCInfo(TURN_TO_PERSON) << "Angular gain: " << m_angular_gain;
    yCInfo(TURN_TO_PERSON) << "Max angular velocity: " << m_max_angular_vel << " deg/s";
    yCInfo(TURN_TO_PERSON) << "Dead zone: " << m_dead_zone << " pixels";
    yCInfo(TURN_TO_PERSON) << "Min angular threshold: " << m_min_angular_threshold << " deg/s";
    yCInfo(TURN_TO_PERSON) << "Search angular velocity: " << m_search_angular_vel << " deg/s";
    
    return true;
}

bool TurnToPerson::interruptModule()
{
    yCInfo(TURN_TO_PERSON) << "Interrupting module";
    m_keypointsInputPort.interrupt();
    m_rpcPort.interrupt();
    m_velocityCommandPort.interrupt();
    
    // Stop the robot
    sendDirectVelocityCommand(0.0);
    
    return true;
}

double TurnToPerson::getPeriod()
{
    return m_period;
}

bool TurnToPerson::updateModule()
{
    if (!m_active)
    {
        return true;
    }
    
    // Read keypoints from input port
    yarp::os::Bottle* keypointsBottle = m_keypointsInputPort.read(false);
    if (keypointsBottle == nullptr)
    {
        return true; // No new data
    }
    
    PersonKeypoints person;
    if (!parseKeypoints(keypointsBottle, person))
    {
        yCDebug(TURN_TO_PERSON) << "No valid person detected - searching...";
        // Turn slowly to search for a person
        sendDirectVelocityCommand(m_search_angular_vel);
        yCDebug(TURN_TO_PERSON) << "Searching with angular velocity: " << m_search_angular_vel << " deg/s";
        return true;
    }
    
    if (!calculateCentroid(person))
    {
        yCDebug(TURN_TO_PERSON) << "Cannot calculate person centroid - searching...";
        // Turn slowly to search for a person
        sendDirectVelocityCommand(m_search_angular_vel);
        yCDebug(TURN_TO_PERSON) << "Searching with angular velocity: " << m_search_angular_vel << " deg/s";
        return true;
    }
    
    yCDebug(TURN_TO_PERSON) << "Person centroid: (" << person.centroid_u << ", " << person.centroid_v << ")";
    
    // Calculate angular velocity to turn towards person
    double angular_vel = calculateAngularVelocity(person.centroid_u);
    
    // Send velocity command using configurable threshold
    if (fabs(angular_vel) > m_min_angular_threshold)
    {
        sendDirectVelocityCommand(angular_vel);
        yCDebug(TURN_TO_PERSON) << "Turning towards person with angular velocity: " << angular_vel << " deg/s";
    }
    else
    {
        sendDirectVelocityCommand(0.0); // Stop rotation
        yCDebug(TURN_TO_PERSON) << "Person centered, no rotation needed";
    }
    
    return true;
}

bool TurnToPerson::close()
{
    yCInfo(TURN_TO_PERSON) << "Closing module";
    
    // Stop the robot
    sendDirectVelocityCommand(0.0);
    
    // Close ports
    m_keypointsInputPort.close();
    m_rpcPort.close();
    m_velocityCommandPort.close();
    
    return true;
}

bool TurnToPerson::parseKeypoints(const yarp::os::Bottle* keypointsBottle, PersonKeypoints& person)
{
    if (keypointsBottle == nullptr || keypointsBottle->size() == 0)
    {
        return false;
    }
    
    person.valid = false;
    
    // Parse bottle containing bounding box and keypoints
    // Expected format: (("bbox" <left> <right> <top> <bottom>) (kp1 ...) ...)

    if (keypointsBottle->size() > 0)
    {
        yarp::os::Bottle* extBottle = keypointsBottle->get(0).asList();

        if (!extBottle)
        {
            return false;
        }

        yarp::os::Bottle* firstPerson = extBottle->get(0).asList();

        if (firstPerson != nullptr && firstPerson->size() > 0)
        {
            // Look for the bounding box information in the first element
            yarp::os::Bottle* bboxBottle = firstPerson->get(0).asList();
            
            if (bboxBottle != nullptr && bboxBottle->size() >= 5)
            {
                std::string bbox_label = bboxBottle->get(0).asString();
                
                if (bbox_label == "bbox")
                {
                    person.left = bboxBottle->get(1).asFloat64();
                    person.right = bboxBottle->get(2).asFloat64();
                    person.top = bboxBottle->get(3).asFloat64();
                    person.bottom = bboxBottle->get(4).asFloat64();
                    
                    // Validate bounding box coordinates
                    if (person.left >= 0 && person.right > person.left && 
                        person.top >= 0 && person.bottom > person.top)
                    {
                        person.valid = true;
                        yCDebug(TURN_TO_PERSON) << "Parsed bounding box: left=" << person.left 
                                               << ", right=" << person.right 
                                               << ", top=" << person.top 
                                               << ", bottom=" << person.bottom;
                    }
                }
            }
        }
    }

    return person.valid;
}

bool TurnToPerson::calculateCentroid(PersonKeypoints& person)
{
    if (!person.valid)
    {
        return false;
    }
    
    // Calculate centroid from bounding box
    // For horizontal centering, use the middle point between left and right
    person.centroid_u = (person.left + person.right) / 2.0;
    person.centroid_v = (person.top + person.bottom) / 2.0;
    
    yCDebug(TURN_TO_PERSON) << "Calculated centroid from bounding box: (" 
                           << person.centroid_u << ", " << person.centroid_v << ")";
    
    return true;
}

double TurnToPerson::calculateAngularVelocity(double centroid_u)
{
    // Calculate error from image center
    double error_u = centroid_u - m_image_center_u;
    
    // Check if within dead zone
    if (fabs(error_u) < m_dead_zone)
    {
        return 0.0;
    }
    
    // Calculate proportional angular velocity
    // Positive error (person to the right) -> turn right (negative angular velocity)
    // Negative error (person to the left) -> turn left (positive angular velocity)
    double angular_vel = -m_angular_gain * error_u;
    
    // Limit angular velocity
    if (angular_vel > m_max_angular_vel)
    {
        angular_vel = m_max_angular_vel;
    }
    else if (angular_vel < -m_max_angular_vel)
    {
        angular_vel = -m_max_angular_vel;
    }
    
    return angular_vel;
}

bool TurnToPerson::sendVelocityCommand(double angular_vel)
{
    // Send velocity command directly via RPC
    return sendDirectVelocityCommand(angular_vel);
}

bool TurnToPerson::sendDirectVelocityCommand(double angular_vel)
{
    // Use RPC to send velocity command to baseControl2's command input interface
    yarp::os::Bottle cmd, reply;
    
    // Build the RPC command: applyVelocityCommandRPC x_vel y_vel theta_vel timeout
    cmd.addString("applyVelocityCommandRPC");
    cmd.addFloat64(0.0);    // x_vel
    cmd.addFloat64(0.0);    // y_vel  
    cmd.addFloat64(angular_vel); // theta_vel (already in deg/s, baseControl expects deg/s)
    cmd.addFloat64(100.0);  // timeout parameter
    
    // Send RPC command to baseControl2's command input with timeout
    yarp::os::ContactStyle style;
    style.timeout = 1.0; // 1 second timeout
    style.carrier = "tcp";
    
    bool success = yarp::os::Network::write(yarp::os::Contact("/baseControl/input/command/rpc:i"),
                                           cmd, reply, style);
    
    if (success)
    {
        yCInfo(TURN_TO_PERSON) << "Sent velocity command via RPC: angular_vel =" << angular_vel << "deg/s";
        yCDebug(TURN_TO_PERSON) << "RPC command:" << cmd.toString();
        if (reply.size() > 0)
        {
            yCDebug(TURN_TO_PERSON) << "RPC reply:" << reply.toString();
        }
        return true;
    }
    else
    {
        yCError(TURN_TO_PERSON) << "Failed to send velocity command via RPC to baseControl2";
        yCError(TURN_TO_PERSON) << "RPC command was:" << cmd.toString();
        return false;
    }
}

bool TurnToPerson::stopRobot()
{
    return sendDirectVelocityCommand(0.0);
}

bool TurnToPerson::read(yarp::os::ConnectionReader& reader)
{
    yarp::os::Bottle command, reply;
    if (!command.read(reader))
    {
        return false;
    }
    
    respond(command, reply);
    
    yarp::os::ConnectionWriter* writer = reader.getWriter();
    if (writer != nullptr)
    {
        reply.write(*writer);
    }
    
    return true;
}

bool TurnToPerson::respond(const yarp::os::Bottle& command, yarp::os::Bottle& reply)
{
    reply.clear();
    
    if (command.size() == 0)
    {
        reply.addString("error: empty command");
        return true;
    }
    
    std::string cmd = command.get(0).asString();
    
    if (cmd == "help")
    {
        reply.addString("Available commands:");
        reply.addString("- start: Start person tracking");
        reply.addString("- stop: Stop person tracking");
        reply.addString("- get status: Get current module status");
        reply.addString("- set angular_gain <value>: Set angular gain");
        reply.addString("- set max_angular_vel <value>: Set maximum angular velocity");
        reply.addString("- set dead_zone <value>: Set dead zone size");
        reply.addString("- quit: Stop the module");
    }
    else if (cmd == "start")
    {
        m_active = true;
        reply.addString("Person tracking started");
        yCInfo(TURN_TO_PERSON) << "Person tracking started";
    }
    else if (cmd == "stop")
    {
        m_active = false;
        sendDirectVelocityCommand(0.0);
        reply.addString("Person tracking stopped");
        yCInfo(TURN_TO_PERSON) << "Person tracking stopped";
    }
    else if (cmd == "get" && command.size() > 1)
    {
        std::string param = command.get(1).asString();
        if (param == "status")
        {
            reply.addString(m_active ? "active" : "inactive");
        }
        else
        {
            reply.addString("error: unknown parameter");
        }
    }
    else if (cmd == "set" && command.size() > 2)
    {
        std::string param = command.get(1).asString();
        double value = command.get(2).asFloat64();
        
        if (param == "angular_gain")
        {
            m_angular_gain = value;
            reply.addString("angular_gain set to " + std::to_string(value));
        }
        else if (param == "max_angular_vel")
        {
            m_max_angular_vel = value;
            reply.addString("max_angular_vel set to " + std::to_string(value));
        }
        else if (param == "dead_zone")
        {
            m_dead_zone = value;
            reply.addString("dead_zone set to " + std::to_string(value));
        }
        else
        {
            reply.addString("error: unknown parameter");
        }
    }
    else if (cmd == "quit")
    {
        reply.addString("Stopping module");
        return false; // This will stop the module
    }
    else
    {
        reply.addString("error: unknown command. Type 'help' for available commands");
    }
    
    return true;
}
