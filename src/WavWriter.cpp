#include "WavWriter.h"
#include <cstring>
#include <fstream>

WavWriter::WavWriter(const std::string& filename, uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample)
    : m_filename(filename)
    , m_file(nullptr)
    , m_bytesWritten(0)
    , m_isOpen(false) {
    
    memset(&m_header, 0, sizeof(m_header));
    
    memcpy(m_header.riff, "RIFF", 4);
    memcpy(m_header.wave, "WAVE", 4);
    memcpy(m_header.fmt, "fmt ", 4);
    memcpy(m_header.data, "data", 4);
    
    m_header.fmtChunkSize = 16;
    m_header.audioFormat = 1;
    m_header.channels = channels;
    m_header.sampleRate = sampleRate;
    m_header.bitsPerSample = bitsPerSample;
    m_header.blockAlign = (channels * bitsPerSample) / 8;
    m_header.byteRate = sampleRate * m_header.blockAlign;
}

WavWriter::~WavWriter() {
    finalize();
}

bool WavWriter::initialize() {
    m_file = fopen(m_filename.c_str(), "wb");
    if (!m_file) {
        return false;
    }
    
    m_isOpen = true;
    m_bytesWritten = 0;
    writeHeader();
    return true;
}

void WavWriter::write(const uint8_t* data, uint32_t size) {
    if (!m_isOpen || !m_file || !data) {
        return;
    }
    
    fwrite(data, 1, size, m_file);
    m_bytesWritten += size;
}

void WavWriter::finalize() {
    if (m_isOpen && m_file) {
        updateHeader();
        fclose(m_file);
        m_file = nullptr;
        m_isOpen = false;
    }
}

bool WavWriter::isOpen() const {
    return m_isOpen;
}

void WavWriter::writeHeader() {
    if (!m_file) return;
    
    fwrite(&m_header, sizeof(m_header), 1, m_file);
}

void WavWriter::updateHeader() {
    if (!m_file) return;
    
    m_header.dataSize = m_bytesWritten;
    m_header.fileSize = sizeof(m_header) - 8 + m_bytesWritten;
    
    fseek(m_file, 0, SEEK_SET);
    fwrite(&m_header, sizeof(m_header), 1, m_file);
}