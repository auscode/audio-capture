#include <iostream>
#include <memory>
#include "LoopbackCapture.h"
#include "MicCapture.h"
#include "Utils.h"

int main() {
    std::unique_ptr<LoopbackCapture> speakerCapture;
    std::unique_ptr<MicCapture> micCapture;

    std::cout << "Starting audio capture..." << std::endl;
    std::cout << "This will capture both speaker output and microphone input" << std::endl;
    std::cout << "Audio will be saved to output/speaker.wav and output/mic.wav" << std::endl;
    std::cout << "Press Ctrl+C to stop capture" << std::endl;
    std::cout << "---" << std::endl;

    speakerCapture = std::make_unique<LoopbackCapture>();
    micCapture = std::make_unique<MicCapture>();

    if (!speakerCapture->start()) {
        std::cerr << "Failed to start speaker capture" << std::endl;
        micCapture->stop();
        return 1;
    }

    if (!micCapture->start()) {
        std::cerr << "Failed to start microphone capture" << std::endl;
        speakerCapture->stop();
        return 1;
    }

    std::cout << "Capture started successfully!" << std::endl;
    std::cout << "Recording for 30 seconds..." << std::endl;

    for (int i = 30; i > 0; --i) {
        std::cout << "\rTime remaining: " << i << " seconds " << std::flush;
        Utils::sleep(1000);
    }

    std::cout << "\nStopping capture..." << std::endl;

    speakerCapture->stop();
    micCapture->stop();

    std::cout << "Capture complete!" << std::endl;
    std::cout << "Files saved to:" << std::endl;
    std::cout << "  - output/speaker.wav" << std::endl;
    std::cout << "  - output/mic.wav" << std::endl;

    return 0;
}