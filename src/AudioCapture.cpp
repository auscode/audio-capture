#include "AudioCapture.h"
#include "Utils.h"
#include <iostream>

AudioCapture::AudioCapture() {
#ifdef PLATFORM_WINDOWS
    m_hWaveIn = nullptr;
    m_hThread = nullptr;
    m_bRunning = false;
#else
    m_pThread = nullptr;
    m_bRunning = false;
#endif
}

AudioCapture::~AudioCapture() {
    if (isRunning()) {
        // Don't call pure virtual stop() from destructor
        m_bRunning = false;
    }
    cleanup();
}

bool AudioCapture::isRunning() const {
#ifdef PLATFORM_WINDOWS
    return m_bRunning;
#else
    return m_bRunning.load();
#endif
}

#ifdef PLATFORM_WINDOWS
bool AudioCapture::initializeWaveIn() {
    // Set up the wave format
    m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    m_waveFormat.nChannels = 2; // Stereo
    m_waveFormat.nSamplesPerSec = 44100;
    m_waveFormat.wBitsPerSample = 16;
    m_waveFormat.nBlockAlign = (m_waveFormat.nChannels * m_waveFormat.wBitsPerSample) / 8;
    m_waveFormat.nAvgBytesPerSec = m_waveFormat.nSamplesPerSec * m_waveFormat.nBlockAlign;
    m_waveFormat.cbSize = 0;

    return true;
}

void AudioCapture::cleanup() {
    if (m_hWaveIn) {
        waveInClose(m_hWaveIn);
        m_hWaveIn = nullptr;
    }
    
    if (m_hThread) {
        CloseHandle(m_hThread);
        m_hThread = nullptr;
    }
}

DWORD WINAPI AudioCapture::audioThreadProc(LPVOID lpParam) {
    AudioCapture* pThis = static_cast<AudioCapture*>(lpParam);
    if (pThis) {
        pThis->audioThread();
    }
    return 0;
}

void CALLBACK AudioCapture::waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    // Handle wave in messages
}

#else
void AudioCapture::cleanup() {
    if (m_pThread) {
        m_pThread->join();
        delete m_pThread;
        m_pThread = nullptr;
    }
}

void AudioCapture::audioThreadProc(AudioCapture* pThis) {
    if (pThis) {
        pThis->audioThread();
    }
}
#endif