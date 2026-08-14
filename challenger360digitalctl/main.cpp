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


#include "ctlapplication.h"
#include "ctlcommandline.h"

#include "configexceptions.h"
#include "displayexceptions.h"
#include "sensorexceptions.h"

#include <cstdlib>
#include <exception>
#include <iostream>

using namespace challenger360digital;

int main(int argc, char* argv[])
{
    try
    {
        auto options = parseCtlCommandLine(argc, argv);

        return CtlApplication(std::move(options)).run();
    }
    catch (const ConfigException& e)
    {
        std::cerr << "Configuration error: " << e.what() << '\n';

        return EXIT_FAILURE;
    }
    catch (const SensorException& e)
    {
        std::cerr << "Sensor error: " << e.what() << '\n';

        return EXIT_FAILURE;
    }
    catch (const DisplayException& e)
    {
        std::cerr << "Display error: " << e.what() << '\n';

        return EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';

        return EXIT_FAILURE;
    }
}