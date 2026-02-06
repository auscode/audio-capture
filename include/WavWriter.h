#pragma once

#include <string>
#include <cstdint>

class WavWriter {
public:
    WavWriter(const std::string& filename, uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample);
    ~WavWriter();

    bool initialize();
    void write(const uint8_t* data, uint32_t size);
    void finalize();
    bool isOpen() const;

private:
    struct WavHeader {
        char riff[4];
        uint32_t fileSize;
        char wave[4];
        char fmt[4];
        uint32_t fmtChunkSize;
        uint16_t audioFormat;
        uint16_t channels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
        char data[4];
        uint32_t dataSize;
    };

    std::string m_filename;
    WavHeader m_header;
    FILE* m_file;
    uint32_t m_bytesWritten;
    bool m_isOpen;

    void writeHeader();
    void updateHeader();
};