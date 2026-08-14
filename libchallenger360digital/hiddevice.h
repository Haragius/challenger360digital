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
#include <cstdint>
#include <hidapi/hidapi.h>


namespace challenger360digital {

class HidDevice
{
public:
    static constexpr std::size_t ReportSize = 64;

    HidDevice() noexcept = default;
    HidDevice(const HidDevice&) = delete;
    HidDevice(HidDevice&& other) noexcept;

    ~HidDevice() noexcept;

    HidDevice& operator=(HidDevice&& other) noexcept;
    HidDevice& operator=(const HidDevice&) noexcept = delete;

    void open(std::uint16_t vendorId, std::uint16_t productId);
    void close() noexcept;

    [[nodiscard]]
    bool isOpen() const noexcept;

    void send(const std::uint8_t* report);

private:
    hid_device* device_ = nullptr;
};

} // namespace challenger360digital
