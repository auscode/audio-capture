#pragma once

#include "AudioCapture.h"
#include <string>

class MicCapture : public AudioCapture {
public:
    MicCapture(const std::string& outputFile = "output/mic.wav");
    ~MicCapture() override = default;

    bool start() override;
    void stop() override;

private:
    //void audioThread() override;
    std::string m_outputFile;
};