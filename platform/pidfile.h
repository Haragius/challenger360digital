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

#include <filesystem>
#include <sys/types.h>

namespace challenger360digital {

class PidFile
{
public:
    explicit PidFile(std::filesystem::path path);

    PidFile(const PidFile&) = delete;
    PidFile& operator=(const PidFile&) = delete;

    PidFile(PidFile&&) = delete;
    PidFile& operator=(PidFile&&) = delete;

    ~PidFile();

    void create();
    void remove() noexcept;

    [[nodiscard]]
    pid_t read() const;

    [[nodiscard]]
    bool exists() const noexcept;

    [[nodiscard]]
    const std::filesystem::path& path() const noexcept;

private:
    void writePid(int i) const;

    [[nodiscard]]
    static bool processExists(pid_t pid) noexcept;

    std::filesystem::path path_;
    bool created_ = false;
};

}