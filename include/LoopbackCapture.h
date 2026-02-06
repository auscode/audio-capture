#pragma once

#include "AudioCapture.h"
#include <string>

class LoopbackCapture : public AudioCapture {
public:
    LoopbackCapture(const std::string& outputFile = "output/speaker.wav");
    ~LoopbackCapture() override = default;

    bool start() override;
    void stop() override;

private:
    void audioThread() override;
    std::string m_outputFile;
};