// Copyright 2026 Andreas Mettler

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the “Software”), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, but not to sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.

// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.


#include "application.h"

#include "displayexceptions.h"
#include "libchallenger360digital.h"
#include "displaycontroller.h"
#include "signals.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <utility>

namespace challenger360digital {

constexpr auto MainLoopInterval = std::chrono::milliseconds(20);

Application::Application(std::filesystem::path configPath) : configPath_(std::move(configPath))
{
}

void Application::loadConfiguration()
{
    config_ = ApplicationConfig::load(configPath_);
}

void Application::reloadConfiguration()
{
    auto newConfig = ApplicationConfig::load(configPath_);

    config_ = std::move(newConfig);
}

int Application::run()
{
    signals::install();

    loadConfiguration();

    Sensors sensors;

    Display display;
    display.open();

    if (!display.isOpen())
    {
        throw DisplayException("Unable to open HID display");
    }

    DisplayController displayController(sensors, display, config_);

    std::cout << "Challenger 360 Digital started.\n";

    while (!signals::stopRequested)
    {
        if (signals::reloadRequested)
        {
            signals::reloadRequested = 0;

            try
            {
                reloadConfiguration();

                displayController.reloadConfiguration(config_);

                std::cout << "Configuration reloaded." << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Configuration reload failed: " << e.what() << std::endl;
            }
        }

        displayController.tick();

        std::this_thread::sleep_for(MainLoopInterval);
    }

    std::cout << "Challenger 360 Digital stopped.\n";

    return EXIT_SUCCESS;
}

} // namespace challenger360digital