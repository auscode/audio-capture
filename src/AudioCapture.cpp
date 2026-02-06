#include "AudioCapture.h"
#include "Utils.h"
#include <iostream>

AudioCapture::AudioCapture()

#ifdef PLATFORM_WINDOWS
    : m_hWaveIn(nullptr),
      m_hThread(nullptr),
      m_bRunning(false),
      m_writer(nullptr)
#else
    : m_pThread(nullptr),
      m_bRunning(false)
#endif
{
}

AudioCapture::~AudioCapture()
{
    stop();
    cleanup();
}

bool AudioCapture::isRunning() const
{
#ifdef PLATFORM_WINDOWS
    return m_bRunning;
#else
    return m_bRunning.load();
#endif
}

#ifdef PLATFORM_WINDOWS
bool AudioCapture::initializeWaveIn()
{
    // PCM 16-bit stereo 44.1kHz
    m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    m_waveFormat.nChannels = 2;
    m_waveFormat.nSamplesPerSec = 44100;
    m_waveFormat.wBitsPerSample = 16;
    m_waveFormat.nBlockAlign = (m_waveFormat.nChannels * m_waveFormat.wBitsPerSample) / 8;
    m_waveFormat.nAvgBytesPerSec = m_waveFormat.nSamplesPerSec * m_waveFormat.nBlockAlign;
    m_waveFormat.cbSize = 0;

    MMRESULT res = waveInOpen(
        &m_hWaveIn,
        WAVE_MAPPER,
        &m_waveFormat,
        (DWORD_PTR)waveInProc,
        (DWORD_PTR)this,
        CALLBACK_FUNCTION);

    if (res != MMSYSERR_NOERROR)
    {
        std::cerr << "waveInOpen failed\n";
        return false;
    }

    m_writer = new WavWriter(outputFile, 44100, 2, 16);
    if (!m_writer->initialize())
    {
        return false;
    }

    // Prepare buffers
    for (int i = 0; i < BUFFER_COUNT; ++i)
    {
        ZeroMemory(&m_headers[i], sizeof(WAVEHDR));
        m_headers[i].lpData = (LPSTR)m_buffers[i];
        m_headers[i].dwBufferLength = BUFFER_SIZE;

        waveInPrepareHeader(m_hWaveIn, &m_headers[i], sizeof(WAVEHDR));
        waveInAddBuffer(m_hWaveIn, &m_headers[i], sizeof(WAVEHDR));
    }

    return true;
}

void AudioCapture::startInternal()
{
    m_bRunning = true;
    waveInStart(m_hWaveIn);
}

void AudioCapture::stop()
{
    if (!m_bRunning)
        return;

    m_bRunning = false;

    waveInStop(m_hWaveIn);
    waveInReset(m_hWaveIn);

    // for (int i = 0; i < BUFFER_COUNT; ++i)
    // {
    //     waveInUnprepareHeader(m_hWaveIn, &m_headers[i], sizeof(WAVEHDR));
    // }

    if (m_hWaveIn)
    {
        for (int i = 0; i < BUFFER_COUNT; ++i)
        {
            if (m_headers[i].dwFlags & WHDR_PREPARED)
            {
                waveInUnprepareHeader(m_hWaveIn, &m_headers[i], sizeof(WAVEHDR));
            }
        }
    }

    if (m_writer)
    {
        m_writer->finalize();
        delete m_writer;
        m_writer = nullptr;
    }
}

void AudioCapture::cleanup()
{
    if (m_hWaveIn)
    {
        waveInClose(m_hWaveIn);
        m_hWaveIn = nullptr;
    }

    if (m_hThread)
    {
        CloseHandle(m_hThread);
        m_hThread = nullptr;
    }
}

DWORD WINAPI AudioCapture::audioThreadProc(LPVOID lpParam)
{
    AudioCapture *pThis = static_cast<AudioCapture *>(lpParam);
    if (pThis)
    {
        pThis->audioThread();
    }
    return 0;
}

void CALLBACK AudioCapture::waveInProc(
    HWAVEIN,
    UINT uMsg,
    DWORD_PTR dwInstance,
    DWORD_PTR dwParam1,
    DWORD_PTR)
{
    if (uMsg != WIM_DATA)
        return;

    auto *self = reinterpret_cast<AudioCapture *>(dwInstance);
    auto *hdr = reinterpret_cast<WAVEHDR *>(dwParam1);

    if (!self->m_bRunning || !self->m_writer)
        return;

    // 🔴 THIS IS REAL AUDIO
    self->m_writer->write(
        reinterpret_cast<uint8_t *>(hdr->lpData),
        hdr->dwBytesRecorded);

    // Requeue buffer
    waveInAddBuffer(self->m_hWaveIn, hdr, sizeof(WAVEHDR));
}
#else
void AudioCapture::cleanup()
{
    if (m_pThread)
    {
        m_pThread->join();
        delete m_pThread;
        m_pThread = nullptr;
    }
}

void AudioCapture::audioThreadProc(AudioCapture *pThis)
{
    if (pThis)
    {
        pThis->audioThread();
    }
}
#endif