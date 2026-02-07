#include "LoopbackCapture.h"
#include "WavWriter.h"
#include "Utils.h"
#include <iostream>
#include <thread>

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <functiondiscoverykeys_devpkey.h>

LoopbackCapture::LoopbackCapture(const std::string &outputFile)
    : m_outputFile(outputFile) {}

LoopbackCapture::~LoopbackCapture()
{
    stop();
}

bool LoopbackCapture::initialize()
{
    HRESULT hr;

    hr = CoInitialize(nullptr);
    if (FAILED(hr))
    {
        std::cerr << "CoInitialize failed\n";
        return false;
    }

    IMMDeviceEnumerator *enumerator = nullptr;
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void **)&enumerator);
    if (FAILED(hr))
        return false;

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_device);
    enumerator->Release();
    if (FAILED(hr))
        return false;

    hr = m_device->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        (void **)&m_audioClient);
    if (FAILED(hr))
        return false;

    hr = m_audioClient->GetMixFormat(&m_waveFormat);
    if (FAILED(hr))
        return false;

    hr = m_audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        0,
        0,
        m_waveFormat,
        nullptr);
    if (FAILED(hr))
        return false;

    hr = m_audioClient->GetService(
        __uuidof(IAudioCaptureClient),
        (void **)&m_captureClient);
    if (FAILED(hr))
        return false;

    return true;
}

bool LoopbackCapture::start()
{
    Utils::createDirectory("output");

    if (!initialize())
    {
        std::cerr << "Loopback init failed\n";
        return false;
    }

    HRESULT hr = m_audioClient->Start();
    if (FAILED(hr))
        return false;

    m_running = true;
    std::thread(&LoopbackCapture::captureLoop, this).detach();
    return true;
}

void LoopbackCapture::stop()
{
    m_running = false;

    if (m_audioClient)
    {
        m_audioClient->Stop();
        m_audioClient->Release();
        m_audioClient = nullptr;
    }

    if (m_captureClient)
    {
        m_captureClient->Release();
        m_captureClient = nullptr;
    }

    if (m_device)
    {
        m_device->Release();
        m_device = nullptr;
    }

    if (m_waveFormat)
    {
        CoTaskMemFree(m_waveFormat);
        m_waveFormat = nullptr;
    }

    CoUninitialize();
}

void LoopbackCapture::captureLoop()
{
    WavWriter writer(
        m_outputFile,
        m_waveFormat->nSamplesPerSec,
        m_waveFormat->nChannels,
        m_waveFormat->wBitsPerSample);

    if (!writer.initialize())
    {
        std::cerr << "Failed to init WAV writer\n";
        return;
    }

    while (m_running)
    {
        UINT32 packetLength = 0;
        HRESULT hr = m_captureClient->GetNextPacketSize(&packetLength);
        if (FAILED(hr))
            break;

        while (packetLength > 0)
        {
            BYTE *data;
            UINT32 frames;
            DWORD flags;

            hr = m_captureClient->GetBuffer(
                &data,
                &frames,
                &flags,
                nullptr,
                nullptr);
            if (FAILED(hr))
                break;

            if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT))
            {
                writer.write(
                    data,
                    frames * m_waveFormat->nBlockAlign);
            }

            m_captureClient->ReleaseBuffer(frames);
            hr = m_captureClient->GetNextPacketSize(&packetLength);
            if (FAILED(hr))
                break;
        }

        Utils::sleep(5);
    }

    writer.finalize();
}

#endif
