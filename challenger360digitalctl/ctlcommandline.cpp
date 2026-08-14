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


#include "ctlcommandline.h"

#include <algorithm>
#include <cctype>
#include <getopt.h>
#include <stdexcept>
#include <string>
#include <iostream>

namespace challenger360digital {

enum OptionCode
{
    OptionDisplay = 256,
    OptionSensors,
    OptionConfig,
    OptionRaw,
    OptionReload
};

[[nodiscard]]
bool isLayout(const std::string& value) noexcept
{
    return value == "cpu-clock" || value == "rpm" || value == "temperature";
}

[[nodiscard]]
Layout parseLayout(const std::string& value)
{
    if (value == "cpu-clock")
    {
        return Layout::CpuClock;
    }

    if (value == "rpm")
    {
        return Layout::Rpm;
    }

    if (value == "temperature")
    {
        return Layout::Temperature;
    }

    throw std::invalid_argument("Invalid layout '" + value + "'. Expected cpu-clock, rpm or temperature.");
}

[[nodiscard]]
bool isRawMessageValid(const std::string& value)
{
    std::size_t count = 0;
    std::string token;

    for (std::size_t i = 0; i < value.size();)
    {
        while (i < value.size() && std::isspace(static_cast<unsigned char>(value[i])))
        {
            ++i;
        }

        if (i == value.size())
        {
            break;
        }

        const std::size_t start = i;

        while (i < value.size() && !std::isspace(static_cast<unsigned char>(value[i])))
        {
            ++i;
        }

        token = value.substr(start, i - start);

        if (token.empty())
        {
            continue;
        }

        ++count;

        if (count > 6)
        {
            return false;
        }
    }

    return count == 6;
}

void setCommand(CtlCommandLineOptions& options, CtlCommand command)
{
    if (options.command != CtlCommand::None)
    {
        throw std::invalid_argument("Only one command may be specified.");
    }

    options.command = command;
}


bool CtlCommandLineOptions::hasRawMessages() const noexcept
{
    return std::ranges::any_of(rawMessages, [](const auto& message)
        {
            return message.has_value();
        });
}

bool CtlCommandLineOptions::hasDisplayValues() const noexcept
{
    return temperatureSet || cpuClockSet || rpmSet || layoutSet || utilizationSet;
}


CtlCommandLineOptions parseCtlCommandLine(
    int argc,
    char* argv[])
{
    CtlCommandLineOptions options;

    static constexpr option longOptions[] =
        {
            {"help",         no_argument,       nullptr, 'h'},
            {"sensors",      no_argument,       nullptr, OptionSensors},
            {"display",      optional_argument, nullptr, OptionDisplay},
            {"config",       required_argument, nullptr, OptionConfig},
            {"raw",          optional_argument, nullptr, OptionRaw},
            {"reload",       no_argument,       nullptr, OptionReload},

            {"temperature",  required_argument, nullptr, 't'},
            {"cpu-clock",    required_argument, nullptr, 'c'},
            {"rpm",          required_argument, nullptr, 'r'},
            {"layout",       required_argument, nullptr, 'l'},
            {"utilization",  required_argument, nullptr, 'u'},

            {"msg0",         required_argument, nullptr, '0'},
            {"msg1",         required_argument, nullptr, '1'},
            {"msg2",         required_argument, nullptr, '2'},
            {"msg3",         required_argument, nullptr, '3'},
            {"msg4",         required_argument, nullptr, '4'},

            {nullptr, 0, nullptr, 0}
        };

    opterr = 0;

    int optionIndex = 0;

    while (true)
    {
        const int option = getopt_long(argc, argv,"ht:c:r:l:u:0:1:2:3:4:", longOptions, &optionIndex);

        if (option == -1)
            break;

        switch (option)
        {
        case 'h':
            setCommand(options, CtlCommand::Help);
            break;

        case OptionSensors:
            setCommand(options, CtlCommand::Sensors);
            break;

        case OptionDisplay:
            setCommand(options, CtlCommand::Display);
            break;

        case OptionConfig:
            setCommand(options, CtlCommand::Config);
            options.configPath = std::filesystem::path(optarg);
            break;

        case OptionRaw:
            setCommand(options, CtlCommand::Raw);
            break;

        case OptionReload:
            setCommand(options, CtlCommand::Reload);
            break;

        case 't':
            options.temperatureSet = true;
            options.displayState.temperature = std::stof(optarg);
            break;

        case 'c':
            options.cpuClockSet = true;
            options.displayState.cpuClock = std::stoi(optarg);
            break;

        case 'r':
            options.rpmSet = true;
            options.displayState.rpm = std::stoi(optarg);
            break;

        case 'l':
            options.layoutSet = true;
            options.displayState.layout = parseLayout(optarg);
            break;

        case 'u':
            options.utilizationSet = true;
            options.displayState.utilization = std::stoi(optarg);
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        {
            const std::string value = optarg;

            if (!isRawMessageValid(value))
            {
                throw std::invalid_argument("Raw message must contain exactly six hexadecimal bytes.");
            }

            options.rawMessages[static_cast<std::size_t>(option - '0')] = value;

            break;
        }

        case '?':
        default:
            throw std::invalid_argument("Invalid option or missing argument.");
        }
    }

    if (optind < argc)
    {
        throw std::invalid_argument("Unexpected argument: " + std::string(argv[optind]));
    }

    return options;
}


void printCtlHelp(std::string_view programName)
{
    std::cout
        << "\nUsage:\n"
        << "  " << programName << " --help\n"
        << "  " << programName << " --reload\n"
        << "  " << programName << " --sensors\n"
        << "  " << programName << " --config <path>\n"
        << "  " << programName << " --display [options]\n"
        << "  " << programName << " --raw [options]\n"
        << '\n'

        << "Display options:\n"
        << "  -t, --temperature <value>\n"
        << "  -c, --cpu-clock <value>\n"
        << "  -r, --rpm <value>\n"
        << "  -l, --layout <layout>\n"
        << "  -u, --utilization <value>\n"
        << '\n'

        << "Layouts:\n"
        << "  cpu-clock\n"
        << "  rpm\n"
        << "  temperature\n"
        << '\n'

        << "Raw options:\n"
        << "  --msg0 <6 hex bytes>\n"
        << "  --msg1 <6 hex bytes>\n"
        << "  --msg2 <6 hex bytes>\n"
        << "  --msg3 <6 hex bytes>\n"
        << "  --msg4 <6 hex bytes>\n"
        << '\n'

        << "Examples:\n"
        << "  " << programName
        << " --display --temperature 42.5 --rpm 1200\n"

        << "  " << programName
        << " --raw --msg4 '00 00 00 00 02 10'\n"

        << "  " << programName
        << " --reload\n"
        << std::endl;
}

}