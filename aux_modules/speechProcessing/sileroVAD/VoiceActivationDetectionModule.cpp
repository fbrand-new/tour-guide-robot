// SPDX-FileCopyrightText: 2022 Humanoid Sensing and Perception, Istituto Italiano di Tecnologia
// SPDX-License-Identifier: BSD-3-Clause

#include <iostream>
#include "VoiceActivationDetectionModule.h"

YARP_LOG_COMPONENT(VADAUDIOPROCESSORCREATOR, "behavior_tour_robot.voiceActivationDetection.AudioProcessorCreator", yarp::os::Log::TraceType)

bool VoiceActivationDetectionModule::configure(yarp::os::ResourceFinder &rf)
{
    std::string filteredAudioPortOutName = rf.check("filtered_audio_output_port_name",
                                                    yarp::os::Value("/vad/audio:o"),
                                                    "The name of the output port for the filtered audio.")
                                               .asString();

    std::string audioPortIn = rf.check("audio_input_port_name", yarp::os::Value("/vad/audio:i"),
                                       "The name of the input port for the audio.")
                                  .asString();

    std::string wakeWordClientPort = rf.check("wake_wrod_client_port_name", yarp::os::Value("/vad/rpc:o"),
                                            "Name of rpc port to inform wake detector when audio clip is done")
                                       .asString();

    std::string vadServerPort = rf.check("vad_server_port_name", yarp::os::Value("/vad/rpc:i"),
                                                  "Name of the input port for  synchronization rpc port.")
                                             .asString();



    if (!rf.check("vad_frequency", "vad_frequency"))
    {
        yCDebug(VADAUDIOPROCESSORCREATOR) << "Using default 'vad_frequency' parameter of " << VAD_FREQUENCY_DEFAULT;
    }
    else
    {
        m_vadFrequency = rf.find("vad_frequency").asInt32();
    }

    if (!rf.check("vad_threshold", "vad_threshold"))
    {
        yCDebug(VADAUDIOPROCESSORCREATOR) << "Using default 'vad_threshold' parameter of " << VAD_THRESHOLD;
    }
    else
    {
        m_vadThreshold = rf.find("vad_threshold").asFloat32();
    }

    if (!rf.check("vad_gap_allowance", "vad_gap_allowance"))
    {
        yCDebug(VADAUDIOPROCESSORCREATOR) << "Using default 'vad_gap_allowance' parameter of " << VAD_GAP_ALLOWANCE_DEFAULT;
    }
    else
    {
        m_vadGapAllowance = rf.find("vad_gap_allowance").asInt32();
    }

    if (!rf.check("vad_gap_allowance", "vad_gap_allowance"))
    {
        yCDebug(VADAUDIOPROCESSORCREATOR) << "Using default 'vad_save_gap' parameter of " << VAD_SAVE_GAP;
    }
    else
    {
        m_vadSaveGap = rf.find("vad_gap_allowance").asInt32();
    }

    if (!rf.check("vad_gap_allowance", "vad_gap_allowance"))
    {
        yCDebug(VADAUDIOPROCESSORCREATOR) << "Using default 'vad_save_prior_to_detection' parameter of " << VAD_SAVE_PRIOR_TO_DETECTION;
    }
    else
    {
        m_vadSavePriorToDetection = rf.find("vad_gap_allowance").asInt32();
    }

    if (!rf.check("model_path", "model_path"))
    {
        yCDebug(VADAUDIOPROCESSORCREATOR) << "Using default 'model_path' parameter of " << MODEL_PATH;
    }
    else
    {
        m_modelPath = rf.find("model_path").asString();
    }

    if (!m_audioPort.open(audioPortIn))
    {
        yCError(VADAUDIOPROCESSORCREATOR) << "cannot open port " << audioPortIn;
        return false;
    }

    
    m_audioProcessor = std::make_unique<Detector>(m_vadFrequency,
                                                    m_vadGapAllowance,
                                                    m_vadSaveGap,
                                                    m_vadThreshold,
                                                    m_vadSavePriorToDetection,
                                                    m_modelPath,
                                                    filteredAudioPortOutName,
                                                    wakeWordClientPort);

    m_audioPort.useCallback(*m_audioProcessor);
    yCInfo(VADAUDIOPROCESSORCREATOR) << "Started";
    return true;
}

bool VoiceActivationDetectionModule::close()
{
    m_audioPort.close();
    yCInfo(VADAUDIOPROCESSORCREATOR) << "Closing";
    return true;
}

bool VoiceActivationDetectionModule::updateModule()
{
    return true;
}