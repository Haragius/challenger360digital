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


#include "platform/pidfile.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unistd.h>
#include <fstream>
#include <csignal>

namespace challenger360digital {


// =========================================================================
// Constructor / Destructor
// =========================================================================


PidFile::PidFile(std::filesystem::path path) : path_(std::move(path))
{
    if (path_.empty())
    {
        throw std::invalid_argument("PID file path must not be empty");
    }
}

PidFile::~PidFile()
{
    remove();
}


// =========================================================================
// Create PID
// =========================================================================


void PidFile::create()
{
    if (created_)
    {
        return;
    }

    const auto parent = path_.parent_path();

    if (!parent.empty())
    {
        std::error_code ec;

        std::filesystem::create_directories(parent, ec);

        if (ec)
        {
            throw std::runtime_error("Failed to create PID directory '" + parent.string() + "': " + ec.message());
        }
    }

    const int fd = ::open(path_.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0644);

    if (fd >= 0)
    {
        writePid(fd);
        ::close(fd);

        created_ = true;
        return;
    }

    if (errno != EEXIST)
    {
        throw std::runtime_error("Failed to create PID file '" + path_.string() + "': " + std::strerror(errno));
    }

    const pid_t existingPid = read();

    if (processExists(existingPid))
    {
        throw std::runtime_error("Another daemon instance appears to be running (PID " + std::to_string(existingPid) + ')');
    }

    std::error_code ec;

    if (!std::filesystem::remove(path_, ec) || ec)
    {
        throw std::runtime_error("Failed to remove stale PID file '" + path_.string() + "': " + (ec ? ec.message() : "unknown error"));
    }

    const int retryFd = ::open(path_.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0644);

    if (retryFd < 0)
    {
        throw std::runtime_error("Failed to create PID file after removing stale file '" + path_.string() + "': " + std::strerror(errno));
    }

    writePid(retryFd);
    ::close(retryFd);

    created_ = true;
}


// =========================================================================
// Write PID
// =========================================================================


void PidFile::writePid(int fd) const
{
    const std::string pid = std::to_string(static_cast<long long>(::getpid())) + '\n';

    const char* data = pid.data();
    std::size_t remaining = pid.size();

    while (remaining > 0)
    {
        const ssize_t written = ::write(fd, data, remaining);

        if (written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            const int error = errno;

            ::close(fd);

            std::error_code ec;
            std::filesystem::remove(path_, ec);

            throw std::runtime_error("Failed to write PID file '" + path_.string() + "': " + std::strerror(error));
        }

        data += written;
        remaining -= static_cast<std::size_t>(written);
    }
}


// =========================================================================
// Remove PID
// =========================================================================


void PidFile::remove() noexcept
{
    if (!created_)
    {
        return;
    }

    std::error_code ec;

    std::filesystem::remove(path_, ec);

    created_ = false;
}


// =========================================================================
// Read PID
// =========================================================================


pid_t PidFile::read() const
{
    std::ifstream file(path_);

    if (!file)
    {
        throw std::runtime_error("Failed to open PID file '" + path_.string() + "': " + std::strerror(errno));
    }

    long long value = 0;

    file >> value;

    if (!file || value <= 0)
    {
        throw std::runtime_error("Invalid PID in PID file: " + path_.string());
    }

    const pid_t pid = static_cast<pid_t>(value);

    if (static_cast<long long>(pid) != value)
    {
        throw std::runtime_error("PID value is out of range: " + path_.string());
    }

    return pid;
}


// =========================================================================
// Helpers
// =========================================================================


bool PidFile::exists() const noexcept
{
    std::error_code ec;

    return std::filesystem::exists(path_, ec) && !ec;
}

bool PidFile::processExists(pid_t pid) noexcept
{
    if (pid <= 0)
    {
        return false;
    }

    if (::kill(pid, 0) == 0)
    {
        return true;
    }

    return errno == EPERM;
}

const std::filesystem::path& PidFile::path() const noexcept
{
    return path_;
}

}