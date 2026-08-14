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


#include "hiddevice.h"

#include "displayexceptions.h"

#include <utility>

namespace challenger360digital {


// =========================================================================
// Constructor / Destructor
// =========================================================================


HidDevice::~HidDevice() noexcept
{
    close();
}

HidDevice::HidDevice(HidDevice&& other) noexcept : device_(std::exchange(other.device_, nullptr))
{
}


// =========================================================================
// Operator
// =========================================================================


HidDevice& HidDevice::operator=(HidDevice&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    close();

    device_ = std::exchange(other.device_, nullptr);

    return *this;
}


// =========================================================================
// Open / Close
// =========================================================================


void HidDevice::open(const std::uint16_t vendorId, const std::uint16_t productId)
{
    if (isOpen())
    {
        close();
    }

    device_ = hid_open(vendorId, productId, nullptr);

    if (device_ == nullptr)
    {
        throw DisplayConnectionException("Unable to open HID device");
    }
}

void HidDevice::close() noexcept
{
    if (device_ != nullptr)
    {
        hid_close(device_);

        device_ = nullptr;
    }
}


// =========================================================================
// State
// =========================================================================


bool HidDevice::isOpen() const noexcept
{
    return device_ != nullptr;
}


// =========================================================================
// Send
// =========================================================================


void HidDevice::send(const std::uint8_t* report)
{
    if (device_ == nullptr)
    {
        throw DisplayNotOpenException("Cannot send HID report: device is not open");
    }

    if (report == nullptr)
    {
        throw DisplayCommunicationException("Cannot send HID report: report is null");
    }

    const int result = hid_write(device_, report, ReportSize);

    if (result < 0)
    {
        throw DisplayCommunicationException("HID write failed");
    }

    if (result != static_cast<int>(ReportSize))
    {
        throw DisplayCommunicationException("HID write returned an incomplete report");
    }
}

} // namespace challenger360digital