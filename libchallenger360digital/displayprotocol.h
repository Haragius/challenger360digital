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

#include "displaystate.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace challenger360digital {

class Protocol
{
public:

    static constexpr std::size_t ReportSize = 64;
    static constexpr std::size_t ProtocolDataSize = 8;
    static constexpr std::size_t ReportCount = 5;

    using Report = std::array<std::uint8_t, ReportSize>;
    using Reports = std::array<Report, ReportCount>;

    Protocol() noexcept = default;
    ~Protocol() = default;
    Protocol(const Protocol&) = default;
    Protocol(Protocol&&) noexcept = default;

    Protocol& operator=(const Protocol&) = default;
    Protocol& operator=(Protocol&&) noexcept = default;

    [[nodiscard]]
    Reports buildReports(const DisplayState& state) const;

    [[nodiscard]]
    Report buildRaw(std::string_view message) const;

private:
    [[nodiscard]]
    Report buildTemperatureReport(const DisplayState& state) const;

    [[nodiscard]]
    Report buildEmptyReport(std::uint8_t messageId) const;

    [[nodiscard]]
    Report buildCpuClockReport(const DisplayState& state) const;

    [[nodiscard]]
    Report buildRpmReport(const DisplayState& state) const;

    [[nodiscard]]
    static std::uint8_t digit(int value, int divisor) noexcept;

    [[nodiscard]]
    static int temperatureToInteger(double temperature);

    [[nodiscard]]
    static std::uint8_t layoutToProtocolValue(Layout layout);

    [[nodiscard]]
    static std::uint8_t validateUtilization(int utilization);

    static void validateFourDigitValue(int value, std::string_view name);
};

} // namespace challenger360digital