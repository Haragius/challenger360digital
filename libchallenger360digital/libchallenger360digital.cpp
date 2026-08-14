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


#include "libchallenger360digital.h"

#include "displayexceptions.h"

namespace challenger360digital {

// =========================================================================
// Open / Close
// =========================================================================

void Display::open()
{
    if (isOpen())
    {
        return;
    }

    hidDevice_.open(VendorId, ProductId);
}

void Display::close() noexcept
{
    hidDevice_.close();
}

// =========================================================================
// State
// =========================================================================

bool Display::isOpen() const noexcept
{
    return hidDevice_.isOpen();
}

// =========================================================================
// Send reports (0-4 / raw)
// =========================================================================

void Display::update(const DisplayState& state)
{
    if (!isOpen())
    {
        throw DisplayNotOpenException("Cannot update display: display is not open");
    }

    const auto reports = protocol_.buildReports(state);

    for (const auto& report : reports)
    {
        hidDevice_.send(report.data());
    }
}

void Display::sendRawPackage(const std::string_view message)
{
    if (!isOpen())
    {
        throw DisplayNotOpenException("Cannot send raw packet: display is not open");
    }

    const auto report = protocol_.buildRaw(message);

    hidDevice_.send(report.data());
}

} // namespace challenger360digital