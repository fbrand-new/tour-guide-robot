// SPDX-FileCopyrightText: 2022 Humanoid Sensing and Perception, Istituto Italiano di Tecnologia
// SPDX-License-Identifier: BSD-3-Clause

#ifndef VAD_MODULE_H
#define VAD_MODULE_H

#include <yarp/os/RFModule.h>
#include <yarp/dev/AudioRecorderStatus.h>
#include "Detector.h"


class VoiceActivationDetectionModule : public yarp::os::RFModule
{
private:
    static constexpr int VAD_FREQUENCY_DEFAULT = 16000;
    static constexpr float VAD_THRESHOLD = 0.5;
    static constexpr int VAD_GAP_ALLOWANCE_DEFAULT = 5; // In packets of 32 ms
    const std::string MODEL_PATH = "/home/mgonzalez/silero-vad/src/silero_vad/data/silero_vad.onnx";

    int m_vadFrequency{VAD_FREQUENCY_DEFAULT};
    float m_vadThreshold{VAD_THRESHOLD};
    int m_vadGapAllowance{VAD_GAP_ALLOWANCE_DEFAULT};
    std::string m_modelPath;
    yarp::os::BufferedPort<yarp::sig::Sound> m_audioPort;            /** The input port for receiving the microphone input. **/
    std::unique_ptr<Detector> m_audioProcessor;

public:
    bool configure(yarp::os::ResourceFinder &rf) override;
    bool close() override;
    bool updateModule() override;
};

#endif
