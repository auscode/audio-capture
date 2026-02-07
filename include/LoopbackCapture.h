#pragma once

#ifdef PLATFORM_WINDOWS

#include <string>
#include <atomic>
#include <mmdeviceapi.h>
#include <audioclient.h>

class LoopbackCapture
{
public:
    explicit LoopbackCapture(const std::string &outputFile);
    ~LoopbackCapture();

    bool start();
    void stop();

private:
    bool initialize();
    void captureLoop();

private:
    std::string m_outputFile;
    std::atomic<bool> m_running{false};

    IMMDevice *m_device = nullptr;
    IAudioClient *m_audioClient = nullptr;
    IAudioCaptureClient *m_captureClient = nullptr;

    WAVEFORMATEX *m_waveFormat = nullptr;
};

#endif
