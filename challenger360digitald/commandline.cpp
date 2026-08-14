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


#include "commandline.h"

#include <getopt.h>

#include <iostream>
#include <stdexcept>
#include <string>

namespace challenger360digital {

CommandLineOptions parseCommandLine(const int argc, char* argv[])
{
    CommandLineOptions options;

    constexpr option longOptions[] = {
        {"help",   no_argument,       nullptr, 'h'},
        {"config", required_argument, nullptr, 'c'},
        {nullptr,  0,                 nullptr,  0}
    };

    opterr = 0;

    while (true)
    {
        const int result = getopt_long(argc, argv, "hc:", longOptions, nullptr);

        if (result == -1)
        {
            break;
        }

        switch (result)
        {
        case 'h':
            options.help = true;
            break;

        case 'c':

            if (optarg == nullptr || *optarg == '\0')
            {
                throw std::invalid_argument("--config requires a path");
            }

            options.configPath = optarg;
            break;

        case '?':
        default:
            throw std::invalid_argument("invalid command-line option");
        }
    }

    if (optind < argc)
    {
        throw std::invalid_argument("unexpected argument: " + std::string(argv[optind]));
    }

    if (!options.help && options.configPath.empty())
    {
        throw std::invalid_argument("no configuration file specified; use --config <path>");
    }

    return options;
}


void printHelp(std::string_view programName)
{
    std::cout
        << "Usage:\n"
        << "  " << programName
        << " --config <path>\n\n"

        << "Options:\n"
        << "  -h, --help\n"
        << "      Show this help message.\n\n"

        << "  -c, --config <path>\n"
        << "      Path to the configuration file.\n\n"

        << "Signals:\n"
        << "  SIGTERM / SIGINT\n"
        << "      Stop the application.\n\n"

        << "  SIGHUP\n"
        << "      Reload the configuration.\n";
}

} // namespace challenger360digital