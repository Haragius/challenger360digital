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


#include "displaycontroller.h"
#include "displayexceptions.h"
#include "sensorexceptions.h"

#include <sensors/sensors.h>

#include <array>
#include <chrono>

namespace challenger360digital {

constexpr std::chrono::seconds SensorUpdateInterval{1};

constexpr std::array<Layout, 3> Layouts{
    Layout::CpuClock,
    Layout::Rpm,
    Layout::Temperature
};


DisplayController::DisplayController(Sensors& sensors, Display& display, const ApplicationConfig& config) :
    sensors_(sensors),
    display_(display),
    config_(&config),
    lastSensorUpdate_(std::chrono::steady_clock::now()),
    lastLayoutChange_(std::chrono::steady_clock::now())
{
    reloadConfiguration(config);
}


void DisplayController::reloadConfiguration(const ApplicationConfig& config)
{
    config_ = &config;

    selectedSensors_ = sensors_.select(config.sensors());

    if (selectedSensors_.empty())
    {
        throw SensorException("No configured sensors are available");
    }

    currentLayout_ = 0;

    if (config_->updateIntervalS() == 0)
    {
        currentLayout_ = static_cast<std::size_t>(config_->staticLayout());
    }

    state_.layout = layoutFromIndex(currentLayout_);

    const auto now = std::chrono::steady_clock::now();

    lastSensorUpdate_ = now;
    lastLayoutChange_ = now;
}


void DisplayController::tick()
{
    const auto now = std::chrono::steady_clock::now();

    updateLayout(now);

    if (now - lastSensorUpdate_ >= SensorUpdateInterval)
    {
        updateSensors();
        updateDisplay();

        lastSensorUpdate_ = now;
    }
}


void DisplayController::updateLayout(const std::chrono::steady_clock::time_point now)
{
    const int interval = config_->updateIntervalS();

    // 0 means static layout.
    if (interval == 0)
    {
        state_.layout = layoutFromIndex(currentLayout_);
        return;
    }

    const auto layoutInterval = std::chrono::seconds(interval);

    if (now - lastLayoutChange_ < layoutInterval)
    {
        return;
    }

    currentLayout_ = (currentLayout_ + 1) % Layouts.size();

    state_.layout = layoutFromIndex(currentLayout_);

    lastLayoutChange_ = now;
}


Layout DisplayController::layoutFromIndex(const std::size_t index)
{
    return Layouts[index % Layouts.size()];
}


void DisplayController::updateSensors()
{
    sensors_.updateValues(selectedSensors_);

    updateDisplayState();
}


void DisplayController::updateDisplayState()
{
    for (const auto& sensor : selectedSensors_)
    {
        switch (sensor.subfeatureType)
        {
        case SENSORS_SUBFEATURE_TEMP_INPUT:
            state_.temperature = static_cast<float>(sensor.value);
            break;

        case SENSORS_SUBFEATURE_FREQ_INPUT:
            state_.cpuClock = static_cast<int>(sensor.value);
            break;

        case SENSORS_SUBFEATURE_FAN_INPUT:
            state_.rpm = static_cast<int>(sensor.value);
            break;

        default:
            break;
        }
    }
}

void DisplayController::updateDisplay()
{
    display_.update(state_);
}

} // namespace challenger360digital