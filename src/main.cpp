#include <iostream>
#include <memory>

#include "LoopbackCapture.h"
#include "MicCapture.h"
#include "Utils.h"

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "  Audio Capture Application (Windows)" << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;

  auto speakerCapture = std::make_unique<LoopbackCapture>("output/speaker.wav");

  auto micCapture = std::make_unique<MicCapture>("output/mic.wav");

  std::cout << "Starting audio capture..." << std::endl;
  std::cout << "Output files will be saved to:" << std::endl;
  std::cout << "  - output/speaker.wav (system audio)" << std::endl;
  std::cout << "  - output/mic.wav (microphone)" << std::endl;
  std::cout << std::endl;

  std::cout << "=== Starting Speaker Capture ===" << std::endl;
  if (!speakerCapture->start()) {
    std::cerr << std::endl;
    std::cerr << "!!! FAILED to start speaker capture !!!" << std::endl;
    return 1;
  }
  std::cout << std::endl;

  std::cout << "=== Starting Microphone Capture ===" << std::endl;
  if (!micCapture->start()) {
    std::cerr << std::endl;
    std::cerr << "!!! FAILED to start microphone capture !!!" << std::endl;
    std::cout << "Stopping speaker capture..." << std::endl;
    speakerCapture->stop();
    return 1;
  }
  std::cout << std::endl;

  std::cout << "=== Both captures running successfully ===" << std::endl;
  std::cout << "Recording for 30 seconds..." << std::endl;
  std::cout << std::endl;

  for (int i = 30; i > 0; --i) {
    std::cout << "\rTime remaining: " << i << " seconds  " << std::flush;
    Utils::sleep(1000);
  }

  std::cout << std::endl << std::endl;
  std::cout << "=== Stopping Captures ===" << std::endl;
  speakerCapture->stop();
  micCapture->stop();

  std::cout << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << "  Capture Complete!" << std::endl;
  std::cout << "  Check the output folder for WAV files" << std::endl;
  std::cout << "========================================" << std::endl;
  return 0;
}
