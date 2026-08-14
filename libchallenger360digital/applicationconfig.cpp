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


#include "applicationconfig.h"

#include "configexceptions.h"
#include "configparser.h"
#include "sensorexceptions.h"

#include <algorithm>
#include <array>
#include <ranges>
#include <string_view>
#include <utility>
#include "sensors.h"

namespace challenger360digital {


// =========================================================================
// Settings configuration sections
// =========================================================================


constexpr std::string_view SETTINGS_SECTION = "settings";


// =========================================================================
// Settings keys
// =========================================================================


constexpr std::array<std::string_view, 3>
    SETTINGS_KEYS{
        "rotationInterval",
        "staticLayout"
    };


// =========================================================================
// Sensor keys
// =========================================================================


constexpr std::array<std::string_view, 3>
    SENSOR_KEYS{
        "name",
        "label",
        "source"
    };


// =========================================================================
// Sensor sections
// =========================================================================


constexpr std::array<std::string_view, 3>
    SENSOR_SECTIONS{
        "sensor.mhz",
        "sensor.rpm",
        "sensor.temp"
    };


// =========================================================================
// Check whether a string is contained in an array
// =========================================================================


template <typename Range>
[[nodiscard]]
bool contains(const Range& range, std::string_view value)
{
    return std::ranges::find(range, value) != std::ranges::end(range);
}


// =========================================================================
// Load
// =========================================================================


ApplicationConfig ApplicationConfig::load(const std::string& path)
{
    ConfigParser parser(path);

    // ConfigParser performs generic parsing and security checks.
    parser.load();

    // ApplicationConfig performs all application-specific validation.
    ApplicationConfig config(parser);

    config.validate();

    return config;
}


// =========================================================================
// Construction
// =========================================================================


ApplicationConfig::ApplicationConfig(const ConfigParser& parser)
{
    loadApplicationSettings(parser);
    loadSensors(parser);
}


// =========================================================================
// Application settings
// =========================================================================


void ApplicationConfig::loadApplicationSettings(const ConfigParser& parser)
{
    validateSectionKeys(parser, SETTINGS_SECTION);

    staticLayout_ = parser.getInt(SETTINGS_SECTION, "staticLayout", 0);

    updateIntervalS_ = parser.getInt(SETTINGS_SECTION, "rotationInterval");
}


// =========================================================================
// Sensor configuration
// =========================================================================


void ApplicationConfig::loadSensors(const ConfigParser& parser)
{
    sensors_.clear();

    sensors_.reserve(SENSOR_SECTIONS.size());

    for (const auto section : SENSOR_SECTIONS)
    {
        validateSectionKeys(parser, section);

        const std::string chipName = parser.getString(section, "name");

        const std::string label = parser.getString(section, "label");

        const std::string sourceString = parser.getString(section, "source");

        const auto source = sensorSourceFromString(sourceString);

        if (!source)
        {
            throw ConfigValueException("Invalid sensor source '" + sourceString + "' in [" + std::string(section) + "]");
        }

        RequestedSensor request{.source = *source, .chipName = chipName, .label = label
        };

        sensors_.push_back(std::move(request));
    }
}


// =========================================================================
// Validate complete application configuration
// =========================================================================


void ApplicationConfig::validate() const
{
    validateSettings();
    validateSensors();
}


// =========================================================================
// Validate application settings
// =========================================================================


void ApplicationConfig::validateSettings() const
{
    if (staticLayout_ < 0 || staticLayout_ > 2)
    {
        throw ConfigValueException("Settings static layout out of range, 0 | 1 | 2");
    }

    if (updateIntervalS_ < 0 || updateIntervalS_ > 30)
    {
        throw ConfigValueException("Settings update interval out of range. Must be zero or greater but less then 31");
    }
}


// =========================================================================
// Validate sensors
// =========================================================================


void ApplicationConfig::validateSensors() const
{
    if (sensors_.empty())
    {
        throw SensorConfigurationException("No sensors are configured");
    }

    for (const auto& sensor : sensors_)
    {
        if (sensor.chipName.empty())
        {
            throw ConfigValueException("Sensor name cannot be empty");
        }

        if (sensor.label.empty())
        {
            throw ConfigValueException("Sensor label cannot be empty");
        }

        if (sensorSourceToString(sensor.source).empty())
        {
            throw ConfigValueException("Invalid sensor source");
        }

        if (sensor.chipName.size() > 256)
        {
            throw SensorConfigurationException("Configured sensor name is too long: " + sensor.chipName);
        }

        if (sensor.label.size() > 256)
        {
            throw SensorConfigurationException("Configured sensor label is too long: " + sensor.label);
        }
    }
}


// =========================================================================
// Validate keys of a known section
// =========================================================================


void ApplicationConfig::validateSectionKeys(const ConfigParser& parser, std::string_view section)
{
    const auto keys = parser.keys(section);

    if (keys.empty())
    {
        throw ConfigValueException("Configuration section [" + std::string(section) + "] is missing or empty");
    }

    const auto& allowedKeys = section == SETTINGS_SECTION ? SETTINGS_KEYS : SENSOR_KEYS;

    for (const auto& key : keys)
    {
        if (!contains(allowedKeys, key))
        {
            throw ConfigValueException("Unknown configuration key '" + key + "' in [" + std::string(section) + "]");
        }
    }
}


// =========================================================================
// Accessors
// =========================================================================


int ApplicationConfig::staticLayout() const noexcept
{
    return staticLayout_;
}

int
ApplicationConfig::updateIntervalS() const noexcept
{
    return updateIntervalS_;
}

const std::vector<RequestedSensor>& ApplicationConfig::sensors() const noexcept
{
    return sensors_;
}

} // namespace challenger360digital