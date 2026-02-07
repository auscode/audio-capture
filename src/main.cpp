#include <iostream>
#include <memory>

#include "LoopbackCapture.h"
#include "MicCapture.h"
#include "Utils.h"

int main()
{
    auto speakerCapture =
        std::make_unique<LoopbackCapture>("output/speaker.wav");

    auto micCapture =
        std::make_unique<MicCapture>("output/mic.wav");

    std::cout << "Starting audio capture...\n";

    if (!speakerCapture->start())
    {
        std::cerr << "Failed to start speaker capture\n";
        return 1;
    }

    if (!micCapture->start())
    {
        std::cerr << "Failed to start microphone capture\n";
        speakerCapture->stop();
        return 1;
    }

    for (int i = 30; i > 0; --i)
    {
        std::cout << "\rTime remaining: " << i << " seconds" << std::flush;
        Utils::sleep(1000);
    }

    speakerCapture->stop();
    micCapture->stop();

    std::cout << "\nCapture complete\n";
    return 0;
}
