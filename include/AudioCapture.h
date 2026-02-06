#pragma once

#include <string>
#include <cstdint>
#include "WavWriter.h"
class WavWriter;

#ifdef _WIN32
    #include <windows.h>
    #include <objbase.h>
    #include <mmsystem.h>
    #include <thread>
    #include <atomic>
    //#define PLATFORM_WINDOWS
#else
    #include <thread>
    #include <atomic>
    #include <mutex>
    #include <condition_variable>
    #define PLATFORM_LINUX
#endif

class AudioCapture 
public:
    AudioCapture();
    virtual ~AudioCapture();

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const;

    protected:
    void cleanup();
    virtual void audioThread() = 0;
    void startInternal();

    std::string m_outputFile;

#ifdef PLATFORM_WINDOWS
    bool initializeWaveIn();
    HWAVEIN m_hWaveIn;
    HANDLE m_hThread;
    WAVEFORMATEX m_waveFormat;
    bool m_bRunning;
    
    static DWORD WINAPI audioThreadProc(LPVOID lpParam);
    static void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    static constexpr int BUFFER_COUNT = 4;
    static constexpr int BUFFER_SIZE = 4096;

    WAVEHDR m_headers[BUFFER_COUNT];
    BYTE m_buffers[BUFFER_COUNT][BUFFER_SIZE];

    WavWriter *m_writer;
#else
    std::thread* m_pThread;
    std::atomic<bool> m_bRunning;
    static void audioThreadProc(AudioCapture* pThis);
#endif
;