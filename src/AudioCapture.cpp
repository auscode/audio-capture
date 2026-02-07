#include "AudioCapture.h"
#include "Utils.h"
#include <iostream>

AudioCapture::AudioCapture()

#ifdef PLATFORM_WINDOWS
    : m_hWaveIn(nullptr), m_hThread(nullptr), m_bRunning(false),
      m_writer(nullptr)
#else
    : m_pThread(nullptr), m_bRunning(false)
#endif
{
}

AudioCapture::~AudioCapture() { cleanup(); }

bool AudioCapture::isRunning() const {
#ifdef PLATFORM_WINDOWS
  return m_bRunning;
#else
  return m_bRunning.load();
#endif
}

#ifdef PLATFORM_WINDOWS
bool AudioCapture::initializeWaveIn() {
  std::cout
      << "[AudioCapture] Setting up audio format (PCM 16-bit stereo 44.1kHz)..."
      << std::endl;

  // PCM 16-bit stereo 44.1kHz
  m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
  m_waveFormat.nChannels = 2;
  m_waveFormat.nSamplesPerSec = 44100;
  m_waveFormat.wBitsPerSample = 16;
  m_waveFormat.nBlockAlign =
      (m_waveFormat.nChannels * m_waveFormat.wBitsPerSample) / 8;
  m_waveFormat.nAvgBytesPerSec =
      m_waveFormat.nSamplesPerSec * m_waveFormat.nBlockAlign;
  m_waveFormat.cbSize = 0;

  std::cout << "[AudioCapture] Format: " << m_waveFormat.nChannels
            << " channels, " << m_waveFormat.nSamplesPerSec << " Hz, "
            << m_waveFormat.wBitsPerSample << " bits" << std::endl;

  std::cout << "[AudioCapture] Opening WaveIn device..." << std::endl;
  MMRESULT res =
      waveInOpen(&m_hWaveIn, WAVE_MAPPER, &m_waveFormat, (DWORD_PTR)waveInProc,
                 (DWORD_PTR)this, CALLBACK_FUNCTION);

  if (res != MMSYSERR_NOERROR) {
    std::cerr << "[AudioCapture] ERROR: waveInOpen failed with error code: "
              << res << std::endl;
    std::cerr << "[AudioCapture] Error codes: MMSYSERR_ALLOCATED="
              << MMSYSERR_ALLOCATED
              << ", MMSYSERR_BADDEVICEID=" << MMSYSERR_BADDEVICEID
              << ", MMSYSERR_NODRIVER=" << MMSYSERR_NODRIVER
              << ", MMSYSERR_NOMEM=" << MMSYSERR_NOMEM
              << ", WAVERR_BADFORMAT=" << WAVERR_BADFORMAT << std::endl;
    return false;
  }

  std::cout << "[AudioCapture] WaveIn device opened successfully" << std::endl;
  std::cout << "[AudioCapture] Creating WAV writer for: " << m_outputFile
            << std::endl;

  m_writer = new WavWriter(m_outputFile, 44100, 2, 16);
  if (!m_writer->initialize()) {
    std::cerr << "[AudioCapture] ERROR: Failed to initialize WAV writer!"
              << std::endl;
    return false;
  }

  std::cout << "[AudioCapture] WAV writer initialized successfully"
            << std::endl;
  std::cout << "[AudioCapture] Preparing " << BUFFER_COUNT
            << " audio buffers..." << std::endl;

  // Prepare buffers
  for (int i = 0; i < BUFFER_COUNT; ++i) {
    ZeroMemory(&m_headers[i], sizeof(WAVEHDR));
    m_headers[i].lpData = (LPSTR)m_buffers[i];
    m_headers[i].dwBufferLength = BUFFER_SIZE;

    MMRESULT prepRes =
        waveInPrepareHeader(m_hWaveIn, &m_headers[i], sizeof(WAVEHDR));
    if (prepRes != MMSYSERR_NOERROR) {
      std::cerr
          << "[AudioCapture] ERROR: waveInPrepareHeader failed for buffer " << i
          << " with error: " << prepRes << std::endl;
      return false;
    }

    MMRESULT addRes =
        waveInAddBuffer(m_hWaveIn, &m_headers[i], sizeof(WAVEHDR));
    if (addRes != MMSYSERR_NOERROR) {
      std::cerr << "[AudioCapture] ERROR: waveInAddBuffer failed for buffer "
                << i << " with error: " << addRes << std::endl;
      return false;
    }
  }

  std::cout << "[AudioCapture] All buffers prepared successfully" << std::endl;
  return true;
}

void AudioCapture::startInternal() {
  std::cout << "[AudioCapture] Starting WaveIn recording..." << std::endl;
  m_bRunning = true;
  MMRESULT res = waveInStart(m_hWaveIn);
  if (res != MMSYSERR_NOERROR) {
    std::cerr << "[AudioCapture] ERROR: waveInStart failed with error: " << res
              << std::endl;
  } else {
    std::cout << "[AudioCapture] WaveIn recording started successfully"
              << std::endl;
  }
}

void AudioCapture::stop() {
  if (!m_bRunning)
    return;

  m_bRunning = false;

  waveInStop(m_hWaveIn);
  waveInReset(m_hWaveIn);

  // for (int i = 0; i < BUFFER_COUNT; ++i)
  // {
  //     waveInUnprepareHeader(m_hWaveIn, &m_headers[i], sizeof(WAVEHDR));
  // }

  if (m_hWaveIn) {
    for (int i = 0; i < BUFFER_COUNT; ++i) {
      if (m_headers[i].dwFlags & WHDR_PREPARED) {
        waveInUnprepareHeader(m_hWaveIn, &m_headers[i], sizeof(WAVEHDR));
      }
    }
  }

  if (m_writer) {
    m_writer->finalize();
    delete m_writer;
    m_writer = nullptr;
  }
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
  AudioCapture *pThis = static_cast<AudioCapture *>(lpParam);
  if (pThis) {
    pThis->audioThread();
  }
  return 0;
}

void CALLBACK AudioCapture::waveInProc(HWAVEIN, UINT uMsg, DWORD_PTR dwInstance,
                                       DWORD_PTR dwParam1, DWORD_PTR) {
  if (uMsg != WIM_DATA)
    return;

  auto *self = reinterpret_cast<AudioCapture *>(dwInstance);
  auto *hdr = reinterpret_cast<WAVEHDR *>(dwParam1);

  if (!self->m_bRunning || !self->m_writer)
    return;

  static int audioDataCount = 0;
  if (audioDataCount % 100 == 0) { // Print every 100th buffer to avoid spam
    std::cout << "[AudioCapture] Capturing audio data... (buffer #"
              << audioDataCount << ", " << hdr->dwBytesRecorded << " bytes)"
              << std::endl;
  }
  audioDataCount++;

  // 🔴 THIS IS REAL AUDIO
  self->m_writer->write(reinterpret_cast<uint8_t *>(hdr->lpData),
                        hdr->dwBytesRecorded);

  // Requeue buffer
  waveInAddBuffer(self->m_hWaveIn, hdr, sizeof(WAVEHDR));
}
#else
void AudioCapture::cleanup() {
  if (m_pThread) {
    m_pThread->join();
    delete m_pThread;
    m_pThread = nullptr;
  }
}

void AudioCapture::audioThreadProc(AudioCapture *pThis) {
  if (pThis) {
    pThis->audioThread();
  }
}
#endif