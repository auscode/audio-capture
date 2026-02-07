#include "MicCapture.h"
#include "WavWriter.h"
#include "Utils.h"
#include <iostream>

MicCapture::MicCapture(const std::string& outputFile) 
    : m_outputFile(outputFile) {
}

bool MicCapture::start() {
    Utils::createDirectory("output");
    
#ifdef PLATFORM_WINDOWS
    if (!initializeWaveIn()) {
        return false;
    }
    startInternal();
    // Open wave in device for microphone recording
    MMRESULT result = waveInOpen(&m_hWaveIn, WAVE_MAPPER, &m_waveFormat, 
                                (DWORD_PTR)waveInProc, (DWORD_PTR)this, 
                                CALLBACK_FUNCTION);
    
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to open wave in device: " << result << std::endl;
        return false;
    }

    m_bRunning = true;
    m_hThread = CreateThread(nullptr, 0, audioThreadProc, this, 0, nullptr);
    
    return m_hThread != nullptr;
#else
    // Linux stub implementation
    m_bRunning = true;
    m_pThread = new std::thread(audioThreadProc, this);
    return true;
#endif
}

void MicCapture::stop() {
    m_bRunning = false;
    
#ifdef PLATFORM_WINDOWS
    if (m_hWaveIn) {
        waveInStop(m_hWaveIn);
        waveInReset(m_hWaveIn);
    }
    
    if (m_hThread) {
        WaitForSingleObject(m_hThread, 5000);
        CloseHandle(m_hThread);
        m_hThread = nullptr;
    }
#else
    if (m_pThread) {
        m_pThread->join();
        delete m_pThread;
        m_pThread = nullptr;
    }
#endif
}


