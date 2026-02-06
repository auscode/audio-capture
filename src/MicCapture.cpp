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

void MicCapture::audioThread() {
    WavWriter writer(m_outputFile, 44100, 2, 16);
    if (!writer.initialize()) {
        std::cerr << "Failed to initialize WAV writer" << std::endl;
        return;
    }

#ifdef PLATFORM_WINDOWS
    // Windows-specific microphone capture implementation
    const int BUFFER_SIZE = 4096;
    WAVEHDR* waveHeaders = new WAVEHDR[2];
    BYTE* buffers = new BYTE[BUFFER_SIZE * 2];
    
    // Prepare wave headers
    for (int i = 0; i < 2; i++) {
        waveHeaders[i].lpData = (LPSTR)(buffers + i * BUFFER_SIZE);
        waveHeaders[i].dwBufferLength = BUFFER_SIZE;
        waveHeaders[i].dwFlags = 0;
        waveInPrepareHeader(m_hWaveIn, &waveHeaders[i], sizeof(WAVEHDR));
        waveInAddBuffer(m_hWaveIn, &waveHeaders[i], sizeof(WAVEHDR));
    }
    
    // Start recording
    waveInStart(m_hWaveIn);
    
    while (m_bRunning) {
        Utils::sleep(10);
    }
    
    // Clean up
    waveInStop(m_hWaveIn);
    waveInReset(m_hWaveIn);
    
    for (int i = 0; i < 2; i++) {
        waveInUnprepareHeader(m_hWaveIn, &waveHeaders[i], sizeof(WAVEHDR));
    }
    
    delete[] waveHeaders;
    delete[] buffers;
    
    // Write some dummy data for demonstration
    const char silence[BUFFER_SIZE] = {0};
    for (int i = 0; i < 100 && m_bRunning; i++) {
        writer.write((const uint8_t*)silence, BUFFER_SIZE);
        Utils::sleep(10);
    }
#else
    // Linux stub - create a silent WAV file
    const int BUFFER_SIZE = 4096;
    char silence[BUFFER_SIZE] = {0};
    
    for (int i = 0; i < 100 && m_bRunning.load(); i++) {
        writer.write((const uint8_t*)silence, BUFFER_SIZE);
        Utils::sleep(10);
    }
#endif

    writer.finalize();
}