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
#include <string>
#include <string_view>
#include <map>
#include <vector>

namespace challenger360digital {

class ConfigParser
{
public:

    ConfigParser() = default;
    explicit ConfigParser(std::filesystem::path path);

    void load();
    void load(const std::filesystem::path &path);

    void writeToFile() const;
    void writeToFile(const std::filesystem::path& path) const;

    void setConfigPath(std::filesystem::path path);

    void setString(std::string_view section,
                   std::string_view key,
                   std::string_view value);

    void setInt(std::string_view section,
                std::string_view key,
                int value);

    void setFloat(std::string_view section,
                  std::string_view key,
                  float value);

    void setBool(std::string_view section,
                 std::string_view key,
                 bool value);

    [[nodiscard]]
    std::string getString( std::string_view section, std::string_view key) const;

    [[nodiscard]]
    int getInt( std::string_view section, std::string_view key) const;

    [[nodiscard]]
    float getFloat( std::string_view section, std::string_view key) const;

    [[nodiscard]]
    bool getBool( std::string_view section, std::string_view key) const;

    [[nodiscard]]
    std::string getString( std::string_view section, std::string_view key, std::string_view defaultValue) const;

    [[nodiscard]]
    int getInt( std::string_view section, std::string_view key, std::int64_t defaultValue) const;

    [[nodiscard]]
    float getFloat( std::string_view section, std::string_view key, double defaultValue) const;

    [[nodiscard]]
    bool getBool( std::string_view section, std::string_view key, bool defaultValue) const;

    void validate() const;

    [[nodiscard]]
    std::vector<std::string> sections() const;

    [[nodiscard]]
    std::vector<std::string> keys(std::string_view section) const;

    [[nodiscard]]
    bool contains(std::string_view section, std::string_view key) const noexcept;

    void clearData();

private:
    std::filesystem::path configPath_;

    using Section = std::map<std::string, std::string>;
    using Data = std::map<std::string, Section>;
    Data data_;

    static int parseInt( std::string_view value, std::string_view section, std::string_view key);
    static float parseFloat( std::string_view value, std::string_view section, std::string_view key);
    static bool parseBool( std::string_view value, std::string_view section, std::string_view key);
    static std::string normalize( std::string_view value);

    [[nodiscard]]
    const std::string* findValue(std::string_view section, std::string_view key) const;

    static void ltrim(std::string& s);
    static void rtrim(std::string& s);
    static void trim(std::string& s);
};
}