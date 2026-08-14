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


// =========================================================================
//
//  repID  msgID    D0     D1     D2     D3     D4     D5
// ┌──────┬──────┬──────┬──────┬──────┬──────┬──────┬──────┬──────┬
// │ 0x07 │ 0x0? │ 0x?? │ 0x?? │ 0x?? │ 0x?? │ 0x?? │ 0x?? │  ... │
// └──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┴
//
// Displayed values are always sent as low nibble (0-9), except utilization.
//
// =========================================================================


#include "displayprotocol.h"

#include "displayexceptions.h"

#include <charconv>
#include <cmath>
#include <cctype>
#include <system_error>

namespace challenger360digital {

constexpr std::uint8_t REPORT_ID = 0x07;

constexpr std::uint8_t MESSAGE_TEMPERATURE = 0x00;
constexpr std::uint8_t MESSAGE_EMPTY_1     = 0x01;
constexpr std::uint8_t MESSAGE_EMPTY_2     = 0x02;
constexpr std::uint8_t MESSAGE_CPU_CLOCK   = 0x03;
constexpr std::uint8_t MESSAGE_RPM         = 0x04;

constexpr int MAX_FOUR_DIGIT_VALUE = 9999;

constexpr int MAX_UTILIZATION = 0x14;


// =========================================================================
// Build complete display update
// =========================================================================


Protocol::Reports Protocol::buildReports(const DisplayState& state) const
{
    Reports reports{};

    reports[0] = buildTemperatureReport(state);

    reports[1] = buildEmptyReport(MESSAGE_EMPTY_1);

    reports[2] = buildEmptyReport(MESSAGE_EMPTY_2);

    reports[3] = buildCpuClockReport(state);

    reports[4] = buildRpmReport(state);

    return reports;
}


// =========================================================================
// Temperature
// =========================================================================
//
// repID=0x07, msgID=0x00, D0=hundreds place, D1=tens place, D2=ones place, D3=decimal place, D4=unknown, D5=unknown
//


Protocol::Report Protocol::buildTemperatureReport(const DisplayState& state) const
{
    const int value = temperatureToInteger(state.temperature);

    Report report{};

    report[0] = REPORT_ID;
    report[1] = MESSAGE_TEMPERATURE;

    report[2] = digit(value, 1000);
    report[3] = digit(value, 100);
    report[4] = digit(value, 10);
    report[5] = digit(value, 1);

    return report;
}


// =========================================================================
// Empty report
// =========================================================================
//
// Report1 does not require any data, 07 01 00 00 00 00 00 00 seems fine
// Report2 does not require any data, 07 02 00 00 00 00 00 00 seems fine
//

Protocol::Report Protocol::buildEmptyReport(const std::uint8_t messageId) const
{
    Report report{};

    report[0] = REPORT_ID;
    report[1] = messageId;

    return report;
}


// =========================================================================
// CPU clock
// =========================================================================
//
// repID=0x07, msgID=0x03, D0=thousands place, D1=hundreds place, D2=tens place, D3=ones place, D4=unknown, D5=unknown
//


Protocol::Report Protocol::buildCpuClockReport(const DisplayState& state) const
{
    validateFourDigitValue(state.cpuClock, "CPU clock");

    Report report{};

    report[0] = REPORT_ID;
    report[1] = MESSAGE_CPU_CLOCK;

    report[2] = digit(state.cpuClock, 1000);
    report[3] = digit(state.cpuClock, 100);
    report[4] = digit(state.cpuClock, 10);
    report[5] = digit(state.cpuClock, 1);

    return report;
}


// =========================================================================
// RPM / Layout / Utilization
// =========================================================================
//
// repID=0x07, msgID=0x04, D0=thousands place, D1=hundreds place, D2=tens place, D3=ones place, D4=layout, D5=utilization
//


Protocol::Report Protocol::buildRpmReport(const DisplayState& state) const
{
    validateFourDigitValue(state.rpm, "RPM");

    const auto layout = layoutToProtocolValue(state.layout);

    const auto utilization = validateUtilization(state.utilization);

    Report report{};

    report[0] = REPORT_ID;
    report[1] = MESSAGE_RPM;

    report[2] = digit(state.rpm, 1000);
    report[3] = digit(state.rpm, 100);
    report[4] = digit(state.rpm, 10);
    report[5] = digit(state.rpm, 1);

    report[6] = layout;
    report[7] = utilization;

    return report;
}


// =========================================================================
// Raw packet
// =========================================================================

Protocol::Report Protocol::buildRaw(const std::string_view message) const
{
    Report report{};

    std::size_t index = 0;
    std::size_t position = 0;

    while (position < message.size())
    {
        while (position < message.size() && std::isspace(static_cast<unsigned char>(message[position])))
        {
            ++position;
        }

        if (position >= message.size())
        {
            break;
        }

        if (index >= ProtocolDataSize)
        {
            throw DisplayProtocolException("Raw packet contains more than 8 bytes");
        }

        const char* begin = message.data() + position;

        const char* end = message.data() + message.size();

        unsigned int value = 0;

        const auto [ptr, error] = std::from_chars(begin, end, value, 16);

        if (error != std::errc{} || ptr == begin)
        {
            throw DisplayProtocolException("Invalid hexadecimal value in raw packet");
        }

        if (value > 0xFF)
        {
            throw DisplayProtocolException("Raw packet byte exceeds 0xFF");
        }

        if (ptr < end && !std::isspace(static_cast<unsigned char>(*ptr)))
        {
            throw DisplayProtocolException("Invalid raw packet token");
        }

        report[index] = static_cast<std::uint8_t>(value);

        ++index;

        position = static_cast<std::size_t>(ptr - message.data());
    }

    if (index == 0)
    {
        throw DisplayProtocolException("Raw packet is empty");
    }

    return report;
}


// =========================================================================
// Helpers / Validation
// =========================================================================

std::uint8_t Protocol::digit(const int value, const int divisor) noexcept
{
    return static_cast<std::uint8_t>((value / divisor) % 10);
}


int Protocol::temperatureToInteger(const double temperature)
{
    if (!std::isfinite(temperature))
    {
        throw DisplayProtocolException("Temperature is not finite");
    }

    constexpr double MIN_TEMPERATURE = 0.0;
    constexpr double MAX_TEMPERATURE = 999.9;

    if (temperature < MIN_TEMPERATURE || temperature > MAX_TEMPERATURE)
    {
        throw DisplayProtocolException("Temperature cannot be represented by the display protocol");
    }

    const double scaled = std::round(temperature * 10.0);

    if (scaled < 0.0 || scaled > MAX_FOUR_DIGIT_VALUE)
    {
        throw DisplayProtocolException("Temperature conversion overflow");
    }

    return static_cast<int>(scaled);
}


std::uint8_t Protocol::layoutToProtocolValue(const Layout layout)
{
    switch (layout)
    {
    case Layout::CpuClock:
        return 0x01;

    case Layout::Rpm:
        return 0x02;

    case Layout::Temperature:
        return 0x04;
    }

    throw DisplayProtocolException("Invalid display layout");
}


std::uint8_t Protocol::validateUtilization(const int utilization)
{
    if (utilization < 0 || utilization > MAX_UTILIZATION)
    {
        throw DisplayProtocolException("Display utilization must be between 0 and 20");
    }

    return static_cast<std::uint8_t>(utilization);
}


void Protocol::validateFourDigitValue(const int value, const std::string_view name)
{
    if (value < 0 || value > MAX_FOUR_DIGIT_VALUE)
    {
        throw DisplayProtocolException(std::string(name) + " cannot be represented by the display protocol");
    }
}

} // namespace challenger360digital