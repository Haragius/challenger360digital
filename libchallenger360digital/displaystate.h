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

#include <cstdint>

namespace challenger360digital {

// ---------------------------------------------------------------------
// Representing fixed values for layout.
// ---------------------------------------------------------------------

enum class Layout : std::uint8_t
{
    CpuClock    = 0x01,
    Rpm         = 0x02,
    Temperature = 0x04
};

// ---------------------------------------------------------------------
// A state represents all values needed to update the display
// ---------------------------------------------------------------------

struct DisplayState
{
    double temperature = 0.0;
    int cpuClock = 0;
    int rpm = 0;
    int utilization = 0;

    Layout layout = Layout::Rpm;
};

} // namespace challenger360digital