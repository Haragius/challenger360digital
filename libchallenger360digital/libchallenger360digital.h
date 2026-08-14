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

#include "hiddevice.h"
#include "displayprotocol.h"

#include <cstdint>
#include <string_view>

namespace challenger360digital {

class Display
{
public:
    static constexpr std::uint16_t VendorId = 0x26CE;
    static constexpr std::uint16_t ProductId = 0x0A13;

    Display() = default;
    ~Display() noexcept = default;
    Display(const Display&) = delete;
    Display(Display&&) noexcept = default;

    Display& operator=(const Display&) = delete;
    Display& operator=(Display&&) noexcept = default;

    void open();
    void close() noexcept;

    [[nodiscard]]
    bool isOpen() const noexcept;

    void update(const DisplayState& state);
    void sendRawPackage(std::string_view message);

private:
    HidDevice hidDevice_;
    Protocol protocol_;
};

} // namespace challenger360digital