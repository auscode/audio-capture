#include "LoopbackCapture.h"
#include "Utils.h"
#include "WavWriter.h"
#include <iostream>
#include <thread>

#ifdef PLATFORM_WINDOWS

#include <functiondiscoverykeys_devpkey.h>
#include <windows.h>

LoopbackCapture::LoopbackCapture(const std::string &outputFile)
    : m_outputFile(outputFile) {}

LoopbackCapture::~LoopbackCapture() { stop(); }

bool LoopbackCapture::initialize() {
  std::cout << "[LoopbackCapture] Initializing COM..." << std::endl;
  HRESULT hr;

  hr = CoInitialize(nullptr);
  if (FAILED(hr)) {
    std::cerr << "[LoopbackCapture] ERROR: CoInitialize failed with HRESULT: 0x"
              << std::hex << hr << std::dec << std::endl;
    return false;
  }
  std::cout << "[LoopbackCapture] COM initialized successfully" << std::endl;

  std::cout << "[LoopbackCapture] Creating device enumerator..." << std::endl;
  IMMDeviceEnumerator *enumerator = nullptr;
  hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                        __uuidof(IMMDeviceEnumerator), (void **)&enumerator);
  if (FAILED(hr)) {
    std::cerr
        << "[LoopbackCapture] ERROR: CoCreateInstance failed with HRESULT: 0x"
        << std::hex << hr << std::dec << std::endl;
    return false;
  }
  std::cout << "[LoopbackCapture] Device enumerator created" << std::endl;

  std::cout << "[LoopbackCapture] Getting default audio endpoint..."
            << std::endl;
  hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_device);
  enumerator->Release();
  if (FAILED(hr)) {
    std::cerr << "[LoopbackCapture] ERROR: GetDefaultAudioEndpoint failed with "
                 "HRESULT: 0x"
              << std::hex << hr << std::dec << std::endl;
    return false;
  }
  std::cout << "[LoopbackCapture] Default audio endpoint obtained" << std::endl;

  std::cout << "[LoopbackCapture] Activating audio client..." << std::endl;
  hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                          (void **)&m_audioClient);
  if (FAILED(hr)) {
    std::cerr << "[LoopbackCapture] ERROR: Activate failed with HRESULT: 0x"
              << std::hex << hr << std::dec << std::endl;
    return false;
  }
  std::cout << "[LoopbackCapture] Audio client activated" << std::endl;

  std::cout << "[LoopbackCapture] Getting mix format..." << std::endl;
  hr = m_audioClient->GetMixFormat(&m_waveFormat);
  if (FAILED(hr)) {
    std::cerr << "[LoopbackCapture] ERROR: GetMixFormat failed with HRESULT: 0x"
              << std::hex << hr << std::dec << std::endl;
    return false;
  }
  std::cout << "[LoopbackCapture] Mix format: " << m_waveFormat->nChannels
            << " channels, " << m_waveFormat->nSamplesPerSec << " Hz, "
            << m_waveFormat->wBitsPerSample << " bits" << std::endl;

  std::cout << "[LoopbackCapture] Initializing audio client in loopback mode..."
            << std::endl;
  hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                 AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0,
                                 m_waveFormat, nullptr);
  if (FAILED(hr)) {
    std::cerr << "[LoopbackCapture] ERROR: Initialize failed with HRESULT: 0x"
              << std::hex << hr << std::dec << std::endl;
    return false;
  }
  std::cout << "[LoopbackCapture] Audio client initialized in loopback mode"
            << std::endl;

  std::cout << "[LoopbackCapture] Getting capture client service..."
            << std::endl;
  hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient),
                                 (void **)&m_captureClient);
  if (FAILED(hr)) {
    std::cerr << "[LoopbackCapture] ERROR: GetService failed with HRESULT: 0x"
              << std::hex << hr << std::dec << std::endl;
    return false;
  }
  std::cout << "[LoopbackCapture] Capture client service obtained" << std::endl;

  return true;
}

bool LoopbackCapture::start() {
  std::cout << "[LoopbackCapture] Starting speaker capture..." << std::endl;
  Utils::createDirectory("output");

  if (!initialize()) {
    std::cerr << "[LoopbackCapture] ERROR: Loopback initialization failed!"
              << std::endl;
    return false;
  }

  std::cout << "[LoopbackCapture] Starting audio client..." << std::endl;
  HRESULT hr = m_audioClient->Start();
  if (FAILED(hr)) {
    std::cerr << "[LoopbackCapture] ERROR: Audio client Start() failed with "
                 "HRESULT: 0x"
              << std::hex << hr << std::dec << std::endl;
    return false;
  }

  std::cout << "[LoopbackCapture] Creating capture thread..." << std::endl;
  m_running = true;
  std::thread(&LoopbackCapture::captureLoop, this).detach();

  std::cout << "[LoopbackCapture] Speaker capture started successfully!"
            << std::endl;
  return true;
}

void LoopbackCapture::stop() {
  m_running = false;

  if (m_audioClient) {
    m_audioClient->Stop();
    m_audioClient->Release();
    m_audioClient = nullptr;
  }

  if (m_captureClient) {
    m_captureClient->Release();
    m_captureClient = nullptr;
  }

  if (m_device) {
    m_device->Release();
    m_device = nullptr;
  }

  if (m_waveFormat) {
    CoTaskMemFree(m_waveFormat);
    m_waveFormat = nullptr;
  }

  CoUninitialize();
}

void LoopbackCapture::captureLoop() {
  std::cout << "[LoopbackCapture] Capture loop started" << std::endl;

  WavWriter writer(m_outputFile, m_waveFormat->nSamplesPerSec,
                   m_waveFormat->nChannels, m_waveFormat->wBitsPerSample);

  if (!writer.initialize()) {
    std::cerr << "[LoopbackCapture] ERROR: Failed to initialize WAV writer!"
              << std::endl;
    return;
  }

  std::cout << "[LoopbackCapture] WAV writer initialized, starting capture..."
            << std::endl;

  int captureCount = 0;
  while (m_running) {
    UINT32 packetLength = 0;
    HRESULT hr = m_captureClient->GetNextPacketSize(&packetLength);
    if (FAILED(hr))
      break;

    while (packetLength > 0) {
      BYTE *data;
      UINT32 frames;
      DWORD flags;

      hr = m_captureClient->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
      if (FAILED(hr))
        break;

      if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
        writer.write(data, frames * m_waveFormat->nBlockAlign);

        if (captureCount % 100 ==
            0) { // Print every 100th capture to avoid spam
          std::cout << "[LoopbackCapture] Capturing audio... (packet #"
                    << captureCount << ", " << frames << " frames)"
                    << std::endl;
        }
        captureCount++;
      }

      m_captureClient->ReleaseBuffer(frames);
      hr = m_captureClient->GetNextPacketSize(&packetLength);
      if (FAILED(hr))
        break;
    }

    Utils::sleep(5);
  }

  std::cout << "[LoopbackCapture] Finalizing WAV file..." << std::endl;
  writer.finalize();
  std::cout << "[LoopbackCapture] Capture loop finished" << std::endl;
}

#endif
