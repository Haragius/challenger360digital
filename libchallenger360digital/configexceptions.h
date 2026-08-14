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
#include <stdexcept>
#include <string>

namespace challenger360digital {

class ConfigException : public std::runtime_error
{
public:
    explicit ConfigException(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class ConfigFileException : public ConfigException
{
public:
    explicit ConfigFileException(const std::string& message)
        : ConfigException(message)
    {
    }
};

class ConfigParseException : public ConfigException
{
public:
    explicit ConfigParseException(
        const std::string& message,
        std::size_t line = 0)
        : ConfigException(
              line != 0
                  ? message + " (line " +
                        std::to_string(line) + ")"
                  : message)
    {
    }
};

class ConfigValueException : public ConfigException
{
public:
    explicit ConfigValueException(
        const std::string& message)
        : ConfigException(message)
    {
    }
};

class ConfigSecurityException : public ConfigException
{
public:
    explicit ConfigSecurityException(
        const std::string& message,
        std::size_t line = 0)
        : ConfigException(
              line != 0
                  ? message + " (line " +
                        std::to_string(line) + ")"
                  : message)
    {
    }
};

} // namespace challenger360digital