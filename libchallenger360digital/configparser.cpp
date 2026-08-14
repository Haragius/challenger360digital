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


#include "configparser.h"
#include "configexceptions.h"

#include <algorithm>
#include <charconv>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace challenger360digital
{

// =========================================================================
// Configuration limits
// =========================================================================
//
// These limits are deliberately conservative. They protect against malformed
// or unexpectedly large configuration files.
//
// Adjust them if your application genuinely needs larger values.
//


constexpr std::uintmax_t MAX_CONFIG_FILE_SIZE = 32 * 32; // 1 KiB
constexpr std::size_t MAX_LINE_SIZE = 16 * 16; // 0.25 KiB
constexpr std::size_t MAX_SECTION_SIZE = 32;
constexpr std::size_t MAX_KEY_SIZE = 32;
constexpr std::size_t MAX_VALUE_SIZE = 32;


// =========================================================================
// Helper functions
// =========================================================================


bool isWhitespace(unsigned char character)
{
    return std::isspace(character) != 0;
}

bool containsControlCharacter( std::string_view value)
{
    for (const unsigned char character : value)
    {
        if (std::iscntrl(character) != 0)
        {
            return true;
        }
    }
    return false;
}


// =========================================================================
// Construction
// =========================================================================


ConfigParser::ConfigParser(std::filesystem::path path) : configPath_(std::move(path))
{
}


// =========================================================================
// Load
// =========================================================================


void ConfigParser::load()
{
    if (configPath_.empty()) {
        throw ConfigFileException("Configuration path is empty");
    }

    std::ifstream file(configPath_, std::ios::binary);

    if(!file.is_open()){
        throw ConfigFileException("Unable to open configuration file: " + configPath_.string());
    }

    // -------------------------------------------------------------------------
    // Check file size before reading
    // -------------------------------------------------------------------------

    file.seekg(0, std::ios::end);

    const auto fileSize = file.tellg();

    if (fileSize < 0)
    {
        throw ConfigFileException( "Unable to determine configuration file size: " + configPath_.string());
    }

    if (static_cast<std::uintmax_t>(fileSize) > MAX_CONFIG_FILE_SIZE)
    {
        throw ConfigSecurityException( "Configuration file exceeds maximum size of " + std::to_string(MAX_CONFIG_FILE_SIZE) + " bytes");
    }

    file.seekg(0, std::ios::beg);

    // -------------------------------------------------------------------------
    // Parse into a temporary container
    // -------------------------------------------------------------------------

    Data newData;

    std::string line;
    std::string section;

    std::size_t lineNumber = 0;


    while(std::getline(file, line))
    {
        ++lineNumber;

        // ---------------------------------------------------------------------
        // Protect against excessively long lines
        // ---------------------------------------------------------------------

        if (line.size() > MAX_LINE_SIZE)
        {
            throw ConfigSecurityException( "Configuration line exceeds maximum size", lineNumber);
        }

        trim(line);

        // ---------------------------------------------------------------------
        // Empty line
        // ---------------------------------------------------------------------

        if (line.empty())
        {
            continue;
        }

        // ---------------------------------------------------------------------
        // Comments
        // ---------------------------------------------------------------------

        if (line.front() == '#' || line.front() == ';')
        {
            continue;
        }

        // ---------------------------------------------------------------------
        // Section
        // ---------------------------------------------------------------------

        if (line.front() == '[')
        {
            if (line.size() < 2 || line.back() != ']')
            {
                throw ConfigParseException("Mailformed section header", lineNumber);
            }

            section = line.substr(1, line.size() - 2);

            trim(section);

            if (section.empty())
            {
                throw ConfigParseException("Section name cannot be empty", lineNumber);
            }

            if (section.size() > MAX_SECTION_SIZE)
            {
                throw ConfigSecurityException("Section name exceeds maximum size", lineNumber);
            }

            if (containsControlCharacter(section))
            {
                throw ConfigParseException("Section name contains control characters", lineNumber);
            }

            continue;
        }

        // ---------------------------------------------------------------------
        // A key must be inside a section.
        // ---------------------------------------------------------------------

        if (section.empty())
        {
            throw ConfigParseException("Key/Value found outside a section", lineNumber);
        }

        // ---------------------------------------------------------------------
        // Find key/value separator.
        // ---------------------------------------------------------------------

        const auto seperator = line.find('=');

        if (seperator == std::string::npos)
        {
            throw ConfigParseException("Expected Key=Value", lineNumber);
        }

        std::string key = line.substr(0, seperator);
        std::string value = line.substr(seperator + 1);

        trim(key);
        trim(value);

        // ---------------------------------------------------------------------
        // Validate value.
        // ---------------------------------------------------------------------

        if (value.size() > MAX_VALUE_SIZE)
        {
            throw ConfigSecurityException("Configuration value exceeds maximum size", lineNumber);
        }

        if (containsControlCharacter(value))
        {
            throw ConfigParseException("Configuration value contains control characters", lineNumber);
        }

        // ---------------------------------------------------------------------
        // Reject duplicate keys.
        // ---------------------------------------------------------------------

        auto& entries = newData[section];

        if (entries.contains(key))
        {
            throw ConfigParseException("Duplicate key '" + key + "' in section [" + section + "]", lineNumber);
        }

        entries.emplace(std::move(key), std::move(value));
    }

    // -------------------------------------------------------------------------
    // Check for I/O errors.
    // -------------------------------------------------------------------------

    if (file.bad())
    {
        throw ConfigFileException("Error while reading configuration file: " + configPath_.string());
    }

    // -------------------------------------------------------------------------
    // Only replace the current configuration after successful parsing.
    // -------------------------------------------------------------------------

    data_ = std::move(newData);
}


// =========================================================================
// Load from path
// =========================================================================


void ConfigParser::load(const std::filesystem::path& path)
{
    if (path.empty())
    {
        throw ConfigFileException("Configuration path is empty");
    }

    configPath_ = path;

    load();
}

// =========================================================================
// Write
// =========================================================================


void ConfigParser::writeToFile() const
{
    if (configPath_.empty())
    {
        throw ConfigFileException("Configuration path is empty");
    }

    writeToFile(configPath_);
}

void ConfigParser::writeToFile(const std::filesystem::path& path) const
{
    if (path.empty())
    {
        throw ConfigFileException("Configuration path is empty");
    }

    std::ofstream file(path, std::ios::trunc);

    if (!file.is_open())
    {
        throw ConfigFileException("Unable to open configuration file for writing: " + path.string());
    }

    for (const auto& [section, entries] : data_)
    {
        file << '[' << section << "]\n";

        for (const auto& [key, value] : entries)
        {
            file << key << '=' << value << '\n';
        }

        file << '\n';
    }

    if (!file.good())
    {
        throw ConfigFileException("Error while writing configuration file: " + path.string());
    }
}

// =========================================================================
// Configuration path
// =========================================================================

void ConfigParser::setConfigPath(std::filesystem::path path)
{
    if (path.empty())
    {
        throw ConfigSecurityException("Configuration path cannot be empty");
    }

    configPath_ = std::move(path);
}


// =========================================================================
// Setters
// =========================================================================


void ConfigParser::setString(std::string_view section, std::string_view key, std::string_view value)
{
    if (section.empty())
    {
        throw ConfigValueException("Configuration section cannot be empty");
    }

    if (key.empty())
    {
        throw ConfigValueException("Configuration key cannot be empty");
    }

    if (section.size() > MAX_SECTION_SIZE)
    {
        throw ConfigSecurityException("Section name exceeds maximum size");
    }

    if (key.size() > MAX_KEY_SIZE)
    {
        throw ConfigSecurityException("Configuration key exceeds maximum size");
    }

    if (value.size() > MAX_VALUE_SIZE)
    {
        throw ConfigSecurityException("Configuration value exceeds maximum size");
    }

    if (containsControlCharacter(section))
    {
        throw ConfigValueException("Section contains control characters");
    }

    if (containsControlCharacter(key))
    {
        throw ConfigValueException("Key contains control characters");
    }

    if (containsControlCharacter(value))
    {
        throw ConfigValueException("Value contains control characters");
    }

    data_[std::string(section)][std::string(key)] = std::string(value);
}

void ConfigParser::setInt(std::string_view section, std::string_view key, int value)
{
    setString(section, key, std::to_string(value));
}

void ConfigParser::setFloat(std::string_view section, std::string_view key, float value)
{
    if (!std::isfinite(value))
    {
        throw ConfigValueException("Floating-point configuration value must be finite");
    }

    std::ostringstream stream;

    stream.precision(std::numeric_limits<float>::max_digits10);

    stream << value;

    setString(section, key, stream.str());
}

void ConfigParser::setBool( std::string_view section, std::string_view key, bool value)
{
    setString(section, key, value ? "true" : "false");
}

// =========================================================================
// Lookup
// =========================================================================

const std::string* ConfigParser::findValue(std::string_view section, std::string_view key) const
{
    const auto sectionIt = data_.find(std::string(section));

    if (sectionIt == data_.end())
    {
        return nullptr;
    }

    const auto keyIt = sectionIt->second.find(std::string(key));

    if (keyIt == sectionIt->second.end())
    {
        return nullptr;
    }

    return &keyIt->second;
}

// =========================================================================
// String
// =========================================================================

std::string ConfigParser::getString(std::string_view section, std::string_view key) const
{
    const auto* value = findValue(section, key);

    if (value == nullptr)
    {
        throw ConfigValueException("Missing configuration value [" + std::string(section) + "] " + std::string(key));
    }

    return *value;
}

std::string ConfigParser::getString(std::string_view section, std::string_view key, std::string_view defaultValue) const
{
    const auto* value = findValue(section, key);

    if (value == nullptr)
    {
        return std::string(defaultValue);
    }

    return *value;
}

// =========================================================================
// Integer
// =========================================================================

int ConfigParser::parseInt(std::string_view value, std::string_view section, std::string_view key)
{
    if (value.empty())
    {
        throw ConfigValueException("Empty integer value for [" + std::string(section) + "] " + std::string(key));
    }

    int result{};

    const auto [ptr, error] = std::from_chars(value.data(), value.data() + value.size(), result);

    if (error == std::errc::result_out_of_range)
    {
        throw ConfigValueException("Integer value is out of range for [" + std::string(section) + "] " + std::string(key));
    }

    if (error != std::errc{} || ptr != value.data() + value.size())
    {
        throw ConfigValueException("Invalid integer '" + std::string(value) + "' for [" + std::string(section) + "] " + std::string(key));
    }

    return result;
}

int ConfigParser::getInt( std::string_view section, std::string_view key) const
{
    return parseInt(getString(section, key), section, key);
}

int ConfigParser::getInt(std::string_view section, std::string_view key, std::int64_t defaultValue) const
{
    const auto* value = findValue(section, key);

    if (value == nullptr)
    {
        return defaultValue;
    }

    return parseInt( *value, section, key);
}

// =========================================================================
// Float
// =========================================================================

float ConfigParser::parseFloat(std::string_view value, std::string_view section, std::string_view key)
{
    if (value.empty())
    {
        throw ConfigValueException("Empty floating-point value for [" + std::string(section) + "] " + std::string(key));
    }

    std::string temporary(value);

    char* end = nullptr;

    errno = 0;

    const float result = std::strtof( temporary.c_str(), &end);

    if (end == temporary.c_str() || end != temporary.c_str() + temporary.size())
    {
        throw ConfigValueException("Invalid floating-point value '" + std::string(value) + "' for [" + std::string(section) + "] " + std::string(key));
    }

    if (errno == ERANGE || !std::isfinite(result))
    {
        throw ConfigValueException("Floating-point value is out of range for [" + std::string(section) + "] " + std::string(key));
    }

    return result;
}

float ConfigParser::getFloat(std::string_view section, std::string_view key) const
{
    return parseFloat(getString(section, key), section, key);
}

float ConfigParser::getFloat(std::string_view section, std::string_view key, double defaultValue) const
{
    if (!std::isfinite(defaultValue))
    {
        throw ConfigValueException("Default floating-point value must be finite");
    }

    const auto* value = findValue(section, key);

    if (value == nullptr)
    {
        return defaultValue;
    }

    return parseFloat(*value, section, key);
}

// =========================================================================
// Boolean
// =========================================================================

std::string ConfigParser::normalize(std::string_view value)
{
    std::string result(value);

    trim(result);

    std::ranges::transform(result, result.begin(), [](unsigned char character)
                           {
                               return static_cast<char>(std::tolower(character));
                           });

    return result;
}

bool ConfigParser::parseBool(std::string_view value, std::string_view section, std::string_view key)
{
    const auto normalized = normalize(value);

    if (normalized == "true" ||
        normalized == "1" ||
        normalized == "yes" ||
        normalized == "on")
    {
        return true;
    }

    if (normalized == "false" ||
        normalized == "0" ||
        normalized == "no" ||
        normalized == "off")
    {
        return false;
    }

    throw ConfigValueException("Invalid boolean value '" + std::string(value) + "' for [" + std::string(section) + "] " + std::string(key));
}

bool ConfigParser::getBool(std::string_view section, std::string_view key) const
{
    return parseBool(getString(section, key), section, key);
}

bool ConfigParser::getBool(std::string_view section, std::string_view key, bool defaultValue) const
{
    const auto* value = findValue(section, key);

    if (value == nullptr)
    {
        return defaultValue;
    }

    return parseBool(*value, section, key);
}

// =========================================================================
// Sections
// =========================================================================

std::vector<std::string> ConfigParser::sections() const
{
    std::vector<std::string> result;

    result.reserve(data_.size());

    for (const auto& [section, entries] : data_)
    {
        static_cast<void>(entries);

        result.push_back(section);
    }

    std::ranges::sort(result);

    return result;
}


// =========================================================================
// Keys
// =========================================================================


std::vector<std::string> ConfigParser::keys(std::string_view section) const
{
    const auto sectionIt = data_.find(std::string(section));

    if (sectionIt == data_.end())
    {
        return {};
    }

    std::vector<std::string> result;

    result.reserve(sectionIt->second.size());

    for (const auto& [key, value] : sectionIt->second)
    {
        static_cast<void>(value);

        result.push_back(key);
    }

    std::ranges::sort(result);

    return result;
}


// =========================================================================
// contains
// =========================================================================


bool ConfigParser::contains(std::string_view section, std::string_view key) const noexcept
{
    const auto sectionIt = data_.find(std::string(section));

    if (sectionIt == data_.end())
    {
        return false;
    }

    return sectionIt->second.contains(std::string(key));
}


// =========================================================================
// Validation
// =========================================================================


void ConfigParser::validate() const
{

}

// =========================================================================
// Clear
// =========================================================================


void ConfigParser::clearData()
{
    data_.clear();
}


// =========================================================================
// String utilities
// =========================================================================


void ConfigParser::ltrim(std::string& value)
{
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char character) { return !isWhitespace(character);}));
}

void ConfigParser::rtrim(std::string& value)
{
    value.erase(std::find_if( value.rbegin(), value.rend(), [](unsigned char character) { return !isWhitespace(character); }).base(), value.end());
}

void ConfigParser::trim(std::string& value)
{
    ltrim(value);
    rtrim(value);
}

} // namespace challenger360digital