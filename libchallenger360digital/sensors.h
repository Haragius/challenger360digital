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


#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <sensors/sensors.h>

namespace challenger360digital {


enum class SensorSource
{
    LmSensor,
    Sysfs
};


[[nodiscard]]
std::optional<SensorSource> sensorSourceFromString(std::string_view source);

[[nodiscard]]
std::string sensorSourceToString(SensorSource source);


struct SuperIoInfo
{
    bool present = false;
    std::string chip;
};


struct RequestedSensor
{
    SensorSource source;
    std::string chipName;
    std::string label;
};


struct SensorInfo
{
    SensorSource source;

    std::string path;

    std::string chipName;
    std::string label;

    const sensors_chip_name* chipPtr = nullptr;

    sensors_feature_type featureType{};
    sensors_subfeature_type subfeatureType{};

    int featureNumber = 0;
    int subfeatureNumber = 0;

    double value = 0.0;
};


struct SensorEnumeration
{
    std::vector<SensorInfo> sensors;
    SuperIoInfo superIO;
};


[[nodiscard]]
std::string sensorTypeToString(sensors_subfeature_type type);


class Sensors
{
public:

    Sensors();
    ~Sensors();

    Sensors(const Sensors&) = delete; Sensors& operator=(const Sensors&) = delete;
    Sensors(Sensors&&) = delete; Sensors& operator=(Sensors&&) = delete;

    [[nodiscard]]
    const std::vector<SensorInfo>& availableSensors() const noexcept;

    [[nodiscard]]
    SuperIoInfo superIoInfo() const;

    [[nodiscard]]
    SuperIoInfo superIoInfo(const std::string_view& name) const;

    [[nodiscard]]
    std::vector<std::size_t> findByType(sensors_subfeature_type type) const;

    [[nodiscard]]
    std::optional<std::size_t> findSensor(const RequestedSensor& request) const;

    [[nodiscard]]
    std::vector<SensorInfo> select(std::span<const RequestedSensor> requests) const;

    void updateValues(std::span<SensorInfo> sensors) const;

    [[nodiscard]]
    const SensorInfo& sensorAt(std::size_t index) const;

private:

    [[nodiscard]]
    SensorEnumeration enumerateLmSensors() const;

    [[nodiscard]]
    std::optional<SensorInfo> enumerateSysfsCpuFrequency() const;

    [[nodiscard]]
    static bool matches(const RequestedSensor& request, const SensorInfo& sensor) noexcept;

    [[nodiscard]]
    static bool isSuperIoChip(std::string_view chip) noexcept;

    SensorEnumeration enumeration_;
};

} // namespace challenger360digital