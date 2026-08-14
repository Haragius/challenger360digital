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


#include "sensors.h"
#include "sensorexceptions.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>

namespace challenger360digital {
namespace fs = std::filesystem;

constexpr std::string_view SYSFS_CPU_PATH = "/sys/devices/system/cpu";
constexpr std::string_view SYSFS_CPU_FREQ_FILE = "cpufreq/scaling_cur_freq";
constexpr std::size_t MAX_CPU_ENTRIES = 100;

// =========================================================================
// Helper: check CPU directory name
// =========================================================================

[[nodiscard]]
bool isCpuDirectory(const fs::directory_entry& entry)
{
    if (!entry.is_directory())
    {
        return false;
    }

    const std::string name = entry.path().filename().string();

    if (name.size() <= 3)
    {
        return false;
    }

    if (name.compare(0, 3, "cpu") != 0)
    {
        return false;
    }

    for (std::size_t i = 3; i < name.size(); ++i)
    {
        const unsigned char character = static_cast<unsigned char>(name[i]);

        if (!std::isdigit(character))
        {
            return false;
        }
    }

    return true;
}

// =========================================================================
// Helper: read CPU frequency
// =========================================================================

[[nodiscard]]
std::optional<double> readCpuFrequencyMHz(const fs::path& cpuDirectory)
{
    const fs::path frequencyPath = cpuDirectory / SYSFS_CPU_FREQ_FILE;

    std::ifstream file(frequencyPath);

    if (!file.is_open())
    {
        return std::nullopt;
    }

    long long frequencyKHz = 0;

    file >> frequencyKHz;

    if (!file.good() && !file.eof())
    {
        return std::nullopt;
    }

    if (frequencyKHz <= 0)
    {
        return std::nullopt;
    }

    const double frequencyMHz = static_cast<double>(frequencyKHz) / 1000.0;

    if (!std::isfinite(frequencyMHz))
    {
        return std::nullopt;
    }

    return frequencyMHz;
}

// =========================================================================
// Sensor source conversion
// =========================================================================

std::optional<SensorSource> sensorSourceFromString(std::string_view source)
{
    if (source == "lm_sensor")
    {
        return SensorSource::LmSensor;
    }

    if (source == "sysfs")
    {
        return SensorSource::Sysfs;
    }

    return std::nullopt;
}

std::string sensorSourceToString(SensorSource source)
{
    switch (source)
    {
    case SensorSource::LmSensor:
        return "lm_sensor";

    case SensorSource::Sysfs:
        return "sysfs";
    }

    return "unknown";
}

// =========================================================================
// Sensor type conversion
// =========================================================================

std::string sensorTypeToString(sensors_subfeature_type type)
{
    switch (type)
    {
    case SENSORS_SUBFEATURE_TEMP_INPUT:
        return "Temperature";

    case SENSORS_SUBFEATURE_FAN_INPUT:
        return "Fan RPM";

    case SENSORS_SUBFEATURE_IN_INPUT:
        return "Voltage";

    case SENSORS_SUBFEATURE_POWER_INPUT:
        return "Power";

    case SENSORS_SUBFEATURE_ENERGY_INPUT:
        return "Energy";

    case SENSORS_SUBFEATURE_CURR_INPUT:
        return "Current";

    case SENSORS_SUBFEATURE_HUMIDITY_INPUT:
        return "Humidity";

    case SENSORS_SUBFEATURE_INTRUSION_ALARM:
        return "Intrusion";

    case SENSORS_SUBFEATURE_FREQ_INPUT:
        return "Frequency";

    default:
        return "Unknown";
    }
}

// =========================================================================
// Construction
// =========================================================================

Sensors::Sensors()
{
    const int result = sensors_init(nullptr);

    if (result != 0)
    {
        throw SensorInitializationException("Failed to initialize libsensors");
    }

    try
    {
        enumeration_ = enumerateLmSensors();

        if (const auto cpuFrequency = enumerateSysfsCpuFrequency())
        {
            enumeration_.sensors.push_back(*cpuFrequency);
        }
    }
    catch (...)
    {
        // The destructor is not called if the constructor throws.
        sensors_cleanup();
        throw;
    }
}

// =========================================================================
// Destruction
// =========================================================================

Sensors::~Sensors()
{
    sensors_cleanup();
}

// =========================================================================
// Available sensors
// =========================================================================

const std::vector<SensorInfo>& Sensors::availableSensors() const noexcept
{
    return enumeration_.sensors;
}

// =========================================================================
// Sensor access
// =========================================================================

const SensorInfo& Sensors::sensorAt(std::size_t index) const
{
    return enumeration_.sensors.at(index);
}

// =========================================================================
// Super I/O information
// =========================================================================

SuperIoInfo Sensors::superIoInfo() const
{
    return enumeration_.superIO;
}

SuperIoInfo Sensors::superIoInfo(const std::string_view& name) const
{
    SuperIoInfo info;
    if (isSuperIoChip(name))
    {
        info.present = true;
        info.chip = name;
    }
    return info;
}

// =========================================================================
// Enumerate lm_sensors
// =========================================================================

SensorEnumeration Sensors::enumerateLmSensors() const
{
    SensorEnumeration result;

    int chipNumber = 0;

    const sensors_chip_name* chip = nullptr;

    while ((chip = sensors_get_detected_chips(nullptr, &chipNumber)) != nullptr)
    {

        char chipName[256]{};

        const int nameResult = sensors_snprintf_chip_name(chipName, sizeof(chipName), chip);

        if (isSuperIoChip(chipName))
        {
            result.superIO = superIoInfo(chipName);
        }

        if (nameResult < 0)
        {
            throw SensorEnumerationException("Failed to format lm_sensors chip name");
        }

        int featureNumber = 0;

        const sensors_feature* feature = nullptr;

        while ((feature = sensors_get_features(chip, &featureNumber)) != nullptr)
        {
            const char* rawLabel = sensors_get_label(chip, feature);

            std::string label = rawLabel != nullptr ? rawLabel : "";

            if (rawLabel != nullptr)
            {
                free(const_cast<char*>(rawLabel));
            }

            int subfeatureNumber = 0;

            const sensors_subfeature* subfeature = nullptr;

            while ((subfeature = sensors_get_all_subfeatures(chip, feature, &subfeatureNumber)) != nullptr)
            {
                // Only enumerate readable values.
                if ((subfeature->flags & SENSORS_MODE_R) == 0)
                {
                    continue;
                }

                double value = 0.0;

                const int readResult = sensors_get_value(chip, subfeature->number, &value);

                if (readResult != 0)
                {
                    continue;
                }

                if (!std::isfinite(value))
                {
                    continue;
                }

                SensorInfo info;

                info.source = SensorSource::LmSensor;

                info.chipName = chipName;

                info.label = label;

                info.chipPtr = chip;

                info.featureType = feature->type;

                info.subfeatureType = subfeature->type;

                info.featureNumber = feature->number;

                info.subfeatureNumber = subfeature->number;

                info.value = value;

                result.sensors.push_back(std::move(info));
            }
        }
    }
    return result;
}

// =========================================================================
// Enumerate SysFS CPU frequency
// =========================================================================

std::optional<SensorInfo> Sensors::enumerateSysfsCpuFrequency() const
{
    const fs::path cpuPath = fs::path(SYSFS_CPU_PATH);

    std::error_code error;

    if (!fs::exists(cpuPath, error))
    {
        return std::nullopt;
    }

    if (error)
    {
        return std::nullopt;
    }

    if (!fs::is_directory(cpuPath, error))
    {
        return std::nullopt;
    }

    if (error)
    {
        return std::nullopt;
    }

    SensorInfo info;

    info.source = SensorSource::Sysfs;

    info.chipName = "CPU";

    info.label = "cpuinfo_avg_freq";

    info.subfeatureType = SENSORS_SUBFEATURE_FREQ_INPUT;

    info.path = cpuPath.string();

    // SysFS sensors don't have a libsensors chip pointer.
    info.chipPtr = nullptr;

    return info;
}

// =========================================================================
// Find sensors by type
// =========================================================================

std::vector<std::size_t> Sensors::findByType(sensors_subfeature_type type) const
{
    std::vector<std::size_t> result;

    for (std::size_t index = 0; index < enumeration_.sensors.size(); ++index)
    {
        if (enumeration_.sensors[index].subfeatureType == type)
        {
            result.push_back(index);
        }
    }
    return result;
}

// =========================================================================
// Find a configured/requested sensor
// =========================================================================

std::optional<std::size_t> Sensors::findSensor(const RequestedSensor& request) const
{
    for (std::size_t index = 0; index < enumeration_.sensors.size(); ++index)
    {
        if (matches(request, enumeration_.sensors[index]))
        {
            return index;
        }
    }
    return std::nullopt;
}

// =========================================================================
// Select requested sensors
// =========================================================================

std::vector<SensorInfo> Sensors::select(std::span<const RequestedSensor> requests) const
{
    std::vector<SensorInfo> result;

    result.reserve(requests.size());

    for (const auto& request : requests)
    {
        if (request.chipName.empty())
        {
            throw SensorConfigurationException("Sensor request contains an empty chip name");
        }

        if (request.label.empty())
        {
            throw SensorConfigurationException("Sensor request contains an empty label");
        }

        const auto index = findSensor(request);

        if (!index)
        {
            throw SensorConfigurationException("Configured sensor was not found: source='" + sensorSourceToString(request.source) + "', chip='" + request.chipName + "', label='" + request.label + "'");
        }

        result.push_back(enumeration_.sensors.at(*index));
    }

    return result;
}

// =========================================================================
// Match sensor
// =========================================================================

bool Sensors::matches(const RequestedSensor& request, const SensorInfo& sensor) noexcept
{
    return request.source == sensor.source && request.chipName == sensor.chipName && request.label == sensor.label;
}

// =========================================================================
// Update sensor values
// =========================================================================

void Sensors::updateValues(std::span<SensorInfo> sensors) const
{
    for (auto& sensor : sensors)
    {
        switch (sensor.source)
        {
            // -----------------------------------------------------------------
            // lm_sensors
            // -----------------------------------------------------------------

        case SensorSource::LmSensor:
        {
            if (sensor.chipPtr == nullptr)
            {
                throw SensorReadException("lm_sensors sensor has no chip pointer: " + sensor.chipName + " / " + sensor.label);
            }

            double value = 0.0;

            const int result = sensors_get_value( sensor.chipPtr, sensor.subfeatureNumber, &value);

            if (result != 0)
            {
                throw SensorReadException("Failed to read lm_sensors value: " + sensor.chipName + " / " + sensor.label);
            }

            if (!std::isfinite(value))
            {
                throw SensorReadException("lm_sensors returned a non-finite value: " + sensor.chipName + " / " + sensor.label);
            }

            sensor.value = value;

            break;
        }

            // -----------------------------------------------------------------
            // SysFS
            // -----------------------------------------------------------------

        case SensorSource::Sysfs:
        {
            const fs::path basePath = fs::path(sensor.path);

            std::error_code error;

            if (!fs::exists( basePath, error) || error)
            {
                throw SensorReadException("SysFS sensor path is unavailable: " + sensor.path);
            }

            if (!fs::is_directory( basePath, error) || error)
            {
                throw SensorReadException( "SysFS sensor path is not a directory: " + sensor.path);
            }

            double sumMHz = 0.0;
            std::size_t coreCount = 0;

            std::size_t entriesProcessed = 0;

            fs::directory_iterator iterator(basePath, fs::directory_options::skip_permission_denied, error);

            if (error)
            {
                throw SensorReadException("Unable to enumerate SysFS CPU directory: " + sensor.path);
            }

            const fs::directory_iterator end{};

            for (; iterator != end; iterator.increment(error))
            {
                if (error)
                {
                    break;
                }

                ++entriesProcessed;

                if (entriesProcessed > MAX_CPU_ENTRIES)
                {
                    throw SensorReadException("SysFS CPU directory contains too many entries");
                }

                const auto& entry = *iterator;

                if (!isCpuDirectory(entry))
                {
                    continue;
                }

                const auto frequency = readCpuFrequencyMHz(entry.path());

                if (!frequency)
                {
                    // A CPU may temporarily have no readable
                    // frequency information. Skip it.
                    continue;
                }

                sumMHz += *frequency;

                ++coreCount;
            }

            if (error)
            {
                throw SensorReadException("Error while enumerating SysFS CPU directory: " + sensor.path);
            }

            if (coreCount == 0)
            {
                throw SensorReadException("Unable to read CPU frequency from SysFS");
            }

            const double average = sumMHz / static_cast<double>(coreCount);

            if (!std::isfinite(average))
            {
                throw SensorReadException("Calculated CPU frequency is not finite");
            }

            sensor.value = average;

            break;
        }
        }
    }
}

// =========================================================================
// Super I/O detection
// =========================================================================

bool Sensors::isSuperIoChip(std::string_view chip) noexcept
{
    constexpr std::array<std::string_view, 7>
        prefixes{
            "nct",
            "it87",
            "f718",
            "w836",
            "w837",
            "smsc",
            "sch56"
        };

    return std::ranges::any_of(prefixes, [chip](std::string_view prefix)
                               {
                                   return chip.starts_with(prefix);
                               });
}
} // namespace challenger360digital