// SPDX-FileCopyrightText: 2022 Humanoid Sensing and Perception, Istituto Italiano di Tecnologia
// SPDX-License-Identifier: BSD-3-Clause
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <cstdlib>

#include <iostream>
#include <cstdint>
#include "Detector.h"

YARP_LOG_COMPONENT(VADAUDIOPROCESSOR, "behavior_tour_robot.voiceActivationDetection.AudioProcessor", yarp::os::Log::TraceType)

Detector::Detector(int vadFrequency,
                    int vadSampleLength,
                    int gapAllowance,
                    std::string filteredAudioPortOutName,
                    std::string wakeWordClientPort):
                    m_vadFrequency(vadFrequency),
                    m_vadSampleLength(vadSampleLength),
                    m_gapAllowance(gapAllowance) {
    
    init_onnx_model(modelPath);
    m_currentSoundBufferNorm = std::vector<float>(m_vadSampleLength, 0);
    m_currentSoundBuffer = std::vector<int16_t>(m_vadSampleLength, 0);
    m_context = std::vector<float>(64, 0);
    m_fillCount = 0;

    input.resize(m_context.size() + m_vadSampleLength);
    input_node_dims[0] = 1;
    input_node_dims[1] = m_context.size() + m_currentSoundBuffer.size();

    _state.resize(size_state);
    sr.resize(1);
    sr[0] = 16000;

    reset_states();

    if (!m_rpcClientPort.open(wakeWordClientPort)){
        yCError(VADAUDIOPROCESSOR) << "cannot open port" << wakeWordClientPort;
    }
    m_rpcClient.yarp().attachAsClient(m_rpcClientPort);

    if (!m_filteredAudioOutputPort.open(filteredAudioPortOutName)){
        yCError(VADAUDIOPROCESSOR) << "cannot open port" << wakeWordClientPort;
    }
}

void Detector::init_engine_threads(int inter_threads, int intra_threads) {
    // The method should be called in each thread/proc in multi-thread/proc work
    session_options.SetIntraOpNumThreads(intra_threads);
    session_options.SetInterOpNumThreads(inter_threads);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_DISABLE_ALL);
};

void Detector::init_onnx_model(const std::string& model_path) {
    // Init threads = 1 for 
    init_engine_threads(1, 1);
    // Load model
    session = std::make_shared<Ort::Session>(env, model_path.c_str(), session_options);
};

void Detector::reset_states() {
    // Call reset before each audio start
    std::memset(_state.data(), 0.0f, _state.size() * sizeof(float));
};

void Detector::predict(const std::vector<float> &data) {
    // Infer
    // Create ort tensors
    std::copy(m_context.begin(), m_context.end(), input.begin());
    std::copy(m_currentSoundBuffer.begin(), m_currentSoundBuffer.end(), input.begin() + m_context.size()); 
    Ort::Value input_ort = Ort::Value::CreateTensor<float>(
        memory_info, input.data(), input.size(), input_node_dims, 2);
    Ort::Value state_ort = Ort::Value::CreateTensor<float>(
        memory_info, _state.data(), _state.size(), state_node_dims, 3);
    Ort::Value sr_ort = Ort::Value::CreateTensor<int64_t>(
        memory_info, sr.data(), sr.size(), sr_node_dims, 1);

    // Clear and add inputs
    ort_inputs.clear();
    ort_inputs.emplace_back(std::move(input_ort));
    ort_inputs.emplace_back(std::move(state_ort));
    ort_inputs.emplace_back(std::move(sr_ort));

    // Infer
    ort_outputs = session->Run(
        Ort::RunOptions{nullptr},
        input_node_names.data(), ort_inputs.data(), ort_inputs.size(),
        output_node_names.data(), output_node_names.size());

    // Output probability & update h,c recursively
    float speech_prob = ort_outputs[0].GetTensorMutableData<float>()[0];
    std::cout << speech_prob << std::endl;
    float *stateN = ort_outputs[1].GetTensorMutableData<float>();
    std::memcpy(_state.data(), stateN, size_state * sizeof(float));


    // std::cout << speech_prob << std::endl;
    bool isTalking = speech_prob > 0.5;
    if (isTalking) { 
        yCDebug(VADAUDIOPROCESSOR) << "Voice detected adding to send buffer";
        m_soundDetected = true;
        m_soundToSend.push_back(m_currentSoundBuffer);
        m_gapCounter = 0;
    } else {
        if (m_soundDetected)
        {
            ++m_gapCounter;
            if (m_gapCounter > m_gapAllowance)
            {
                yCDebug(VADAUDIOPROCESSOR) << "End of of speech";
                sendSound();
                m_soundToSend.clear();
                m_soundDetected = false;
                m_rpcClient.stop();
                reset_states();
            }
        } 
    }

    // copy last part in to context for next input
    std::copy(
        m_currentSoundBuffer.end() - m_context.size(),
        m_currentSoundBuffer.end(),  
        m_context.begin()                     
    );
};


void Detector::onRead(yarp::sig::Sound& soundReceived) {
    size_t num_samples = soundReceived.getSamples();

    for (size_t i = 0; i < num_samples; i++)
    {
        m_currentSoundBuffer.at(m_fillCount) = soundReceived.get(i);
        m_currentSoundBufferNorm.at(m_fillCount) = static_cast<float>(soundReceived.get(i)) / INT16_MAX;
        ++m_fillCount;
        if (m_fillCount == m_currentSoundBuffer.size()) {
            predict(m_currentSoundBufferNorm);
            m_fillCount = 0;
        }
    } 
    
}


void Detector::sendSound() {
    int packetsWithSound = m_soundToSend.size();

    yarp::sig::Sound& soundToSend = m_filteredAudioOutputPort.prepare();
    yCDebug(VADAUDIOPROCESSOR) << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> sending recorded voice sound";


    int totalPackets = packetsWithSound < m_minSoundSize ? m_minSoundSize : packetsWithSound;
    int numSamples = m_currentSoundBuffer.size() * totalPackets;
    soundToSend.resize(numSamples);
    soundToSend.setFrequency(m_vadFrequency);
    for (size_t p = 0; p < packetsWithSound; ++p)
    {
        for (size_t i = 0; i < m_currentSoundBuffer.size(); i++)
        {
            soundToSend.set(m_soundToSend[p].at(i), i + (p * m_currentSoundBuffer.size()));
        }
    }

    // padding to minimum size
    for (size_t i = packetsWithSound * m_currentSoundBuffer.size(); i < numSamples; i++)
    {
        soundToSend.set(0, i);
    }
    
    m_filteredAudioOutputPort.write();
}

