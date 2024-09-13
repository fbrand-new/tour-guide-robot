// SPDX-FileCopyrightText: 2022 Humanoid Sensing and Perception, Istituto Italiano di Tecnologia
// SPDX-License-Identifier: BSD-3-Clause


#ifndef DETECTOR_H
#define DETECTOR_H

#include <yarp/sig/Sound.h>
#include <yarp/os/TypedReaderCallback.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/RpcClient.h>

#include <functional>
#include <cmath>
#include <deque>
#include <memory>

#include <mutex>
#include "onnxruntime_cxx_api.h"
#include <yarp/os/BufferedPort.h>

class Detector: public yarp::os::TypedReaderCallback<yarp::sig::Sound> {
public:
    Detector(int vadFrequency,
                   int vadSampleLength,
                   int gapAllowance,
                   std::string filteredAudioPortOutName,
                   std::string wakeWordClientPort);
    using TypedReaderCallback<yarp::sig::Sound>::onRead;
    void onRead(yarp::sig::Sound& soundReceived) override;

private:
    int m_vadFrequency;
    int m_vadSampleLength;

    std::string modelPath = "/home/mgonzalez/silero-vad/src/silero_vad/data/silero_vad.onnx";
    Ort::Env env;
    Ort::SessionOptions session_options;
    std::shared_ptr<Ort::Session> session = nullptr;
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeCPU);

    std::deque<std::vector<int16_t>> m_soundToSend; /** Internal sound buffer. **/
    std::vector<float> m_currentSoundBufferNorm;
    std::vector<int16_t> m_currentSoundBuffer;
    std::vector<float> m_context;
    int m_fillCount; // keep track of up to what index the buffer is full
    bool m_soundDetected{false};
    std::string m_filteredAudioPortOutName;
    yarp::os::BufferedPort<yarp::sig::Sound> m_filteredAudioOutputPort; /** The output port for sending the filtered audio. **/
    std::deque<yarp::sig::Sound> m_soundToProcess;
    int m_gapAllowance = 34;
    int m_gapCounter = 0;
    int m_minSoundSize = 0; // how many extra packets to pad, can help with transcription

    yarp::os::RpcClient m_rpcClientPort;

    // Onnx model
    // Inputs
    std::vector<Ort::Value> ort_inputs;
    
    std::vector<const char *> input_node_names = {"input", "state", "sr"};
    std::vector<float> input;
    unsigned int size_state = 2 * 1 * 128; // It's FIXED.
    std::vector<float> _state;
    std::vector<int64_t> sr;

    int64_t input_node_dims[2] = {};
    const int64_t state_node_dims[3] = {2, 1, 128}; 
    const int64_t sr_node_dims[1] = {1};

    // Outputs
    std::vector<Ort::Value> ort_outputs;
    std::vector<const char *> output_node_names = {"output", "stateN"};



    void processPacket();
    void sendSound();
    void init_engine_threads(int inter_threads, int intra_threads);
    void init_onnx_model(const std::string& model_path);
    void reset_states();
    void predict(const std::vector<float> &data);
};

#endif