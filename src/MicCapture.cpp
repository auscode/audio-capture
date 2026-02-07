#include "MicCapture.h"
#include "Utils.h"
#include "WavWriter.h"
#include <iostream>

MicCapture::MicCapture(const std::string &outputFile) : AudioCapture() {
  std::cout << "[MicCapture] Constructor called with output file: "
            << outputFile << std::endl;
  m_outputFile = outputFile; // Set the base class member
}

bool MicCapture::start() {
  std::cout << "[MicCapture] Starting microphone capture..." << std::endl;
  Utils::createDirectory("output");

#ifdef PLATFORM_WINDOWS
  std::cout << "[MicCapture] Initializing WaveIn device..." << std::endl;

  if (!initializeWaveIn()) {
    std::cerr << "[MicCapture] ERROR: initializeWaveIn() failed!" << std::endl;
    return false;
  }

  std::cout << "[MicCapture] WaveIn initialized successfully" << std::endl;
  std::cout << "[MicCapture] Starting internal capture..." << std::endl;

  startInternal();

  std::cout << "[MicCapture] Creating capture thread..." << std::endl;
  m_hThread = CreateThread(nullptr, 0, audioThreadProc, this, 0, nullptr);

  if (m_hThread == nullptr) {
    DWORD error = GetLastError();
    std::cerr << "[MicCapture] ERROR: Failed to create thread! Error code: "
              << error << std::endl;
    return false;
  }

  std::cout << "[MicCapture] Microphone capture started successfully!"
            << std::endl;
  return true;
#else
  // Linux stub implementation
  m_bRunning = true;
  m_pThread = new std::thread(audioThreadProc, this);
  return true;
#endif
}

void MicCapture::stop() {
  std::cout << "[MicCapture] Stopping microphone capture..." << std::endl;
  m_bRunning = false;

#ifdef PLATFORM_WINDOWS
  if (m_hWaveIn) {
    std::cout << "[MicCapture] Stopping WaveIn device..." << std::endl;
    waveInStop(m_hWaveIn);
    waveInReset(m_hWaveIn);
  }

  if (m_hThread) {
    std::cout << "[MicCapture] Waiting for thread to finish..." << std::endl;
    WaitForSingleObject(m_hThread, 5000);
    CloseHandle(m_hThread);
    m_hThread = nullptr;
  }
  std::cout << "[MicCapture] Microphone capture stopped" << std::endl;
#else
  if (m_pThread) {
    m_pThread->join();
    delete m_pThread;
    m_pThread = nullptr;
  }
#endif
}
