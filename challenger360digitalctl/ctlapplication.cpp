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

#include "configparser.h"
#include "platform/pidfile.h"
#include "libchallenger360digital.h"
#include "sensors.h"

#include <csignal>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <unistd.h>

namespace challenger360digital {



constexpr auto DefaultPidFile = "/run/challenger360digital/challenger360digital.pid";

void sendReloadSignal(pid_t pid)
{
    if (::kill(pid, SIGHUP) == 0)
    {
        return;
    }

    throw std::runtime_error(
        "Failed to send SIGHUP to daemon (PID " +
        std::to_string(pid) +
        "): " +
        std::strerror(errno));
}


CtlApplication::CtlApplication(CtlCommandLineOptions options) : options_(std::move(options))
{
}

int CtlApplication::run()
{
    switch (options_.command)
    {
    case CtlCommand::Help:
        return runHelp();

    case CtlCommand::Reload:
        return runReload();

    case CtlCommand::Sensors:
        return runSensors();

    case CtlCommand::Config:
        return runConfig();

    case CtlCommand::Display:
        return runDisplay();

    case CtlCommand::Raw:
        return runRaw();

    case CtlCommand::None:
    default:
        throw std::invalid_argument("No command specified.");
    }
}


int CtlApplication::runHelp() const
{

    printCtlHelp("challenger360digitalctl");

    return EXIT_SUCCESS;
}


int CtlApplication::runReload() const
{
    PidFile pidFile(DefaultPidFile);

    if (!pidFile.exists())
    {
        throw std::runtime_error("Daemon PID file does not exist: " + pidFile.path().string());
    }

    const pid_t pid = pidFile.read();

    sendReloadSignal(pid);

    std::cout << "Configuration reload requested " << "for daemon (PID " << pid << ").\n";

    return EXIT_SUCCESS;
}


int CtlApplication::runSensors() const
{
    Sensors sensors;

    if (!sensors.superIoInfo().present)
    {
        std::cout << "\nNote: I can't find a SuperIO chip. Is a driver missing?\n" << std::endl;
    }
    else
    {
        std::cout << "\nNote: SuperIO chip found, chip name: " << sensors.superIoInfo().chip << "\n" << std::endl;
    }

    std::cout
        << std::left
        << std::setw(25) << "\nCHIP-NAME:"
        << std::setw(25) << "LABEL:"
        << std::setw(20) << "TYPE:\n"
        << std::endl;

    std::cout << std::left << std::string(75, '-') << '\n';

    for (const auto& s : sensors.availableSensors())
    {
        if (s.subfeatureType != SENSORS_SUBFEATURE_TEMP_INPUT &&
            s.subfeatureType != SENSORS_SUBFEATURE_FREQ_INPUT &&
            s.subfeatureType != SENSORS_SUBFEATURE_FAN_INPUT)
        {
            continue;
        }

        std::cout
            << std::left
            << std::setw(25) << s.chipName
            << std::setw(25) << s.label
            << std::setw(20) << sensorTypeToString(s.subfeatureType)
            << std::endl;
    }

    return EXIT_SUCCESS;
}


int CtlApplication::runDisplay() const
{
    Display display;
    display.open();

    if (!display.isOpen())
    {
        throw std::runtime_error("Failed to open display device.");
    }

    display.update(options_.displayState);

    return EXIT_SUCCESS;
}


int CtlApplication::runRaw() const
{
    Display display;
    display.open();

    if (!display.isOpen())
    {
        throw std::runtime_error("Failed to open display device.");
    }

    for (std::size_t i = 0; i < options_.rawMessages.size(); ++i)
    {
        const auto& message = options_.rawMessages[i];

        const std::string packet = "07 0" + std::to_string(i) + ' ' + message.value_or("00 00 00 00 00 00");

        display.sendRawPackage(packet);
    }

    return EXIT_SUCCESS;
}


int CtlApplication::runConfig() const
{
    Sensors sensors;
    ConfigParser parser;

    // Map Sensor - TEMP
    auto candidates = sensors.findByType(SENSORS_SUBFEATURE_TEMP_INPUT);

    std::cout
        << std::left
        << std::setw(5) << "\nID"
        << std::setw(25) << "CHIP-NAME:"
        << std::setw(25) << "LABEL:"
        << std::setw(20) << "TYPE:\n"
        << std::endl;

    std::cout << std::left << std::string(75, '-') << '\n';

    int i = 0;
    for (const auto& s : candidates)
    {
        std::cout
            << std::left
            << std::setw(5) << i
            << std::setw(25) << sensors.sensorAt(s).chipName
            << std::setw(25) << sensors.sensorAt(s).label
            << std::setw(20) << sensorTypeToString(sensors.sensorAt(s).subfeatureType)
            << std::endl;
        i++;
    }

    std::cout << "\nSelect TEMPERATURE Sensor: ";

    int choice = {};
    std::cin >> choice;

    if (choice >= 0 && choice < candidates.size())
    {
        std::cout << "Selected: "
                  << sensors.sensorAt(candidates[choice]).chipName
                  << " "
                  << sensors.sensorAt(candidates[choice]).label
                  << "\n";

        parser.setString("sensor.temp", "name", sensors.sensorAt(candidates[choice]).chipName);
        parser.setString("sensor.temp", "label", sensors.sensorAt(candidates[choice]).label);
        parser.setString("sensor.temp", "source", sensorSourceToString(sensors.sensorAt(candidates[choice]).source));
    }
    else
    {
        return 1;
    }

    // Map Sensor - RPM
    candidates = sensors.findByType(SENSORS_SUBFEATURE_FAN_INPUT);

    std::cout
        << std::left
        << std::setw(5) << "\nID"
        << std::setw(25) << "CHIP-NAME:"
        << std::setw(25) << "LABEL:"
        << std::setw(20) << "TYPE:\n"
        << std::endl;

    std::cout << std::left << std::string(75, '-') << '\n';

    i = 0;
    for (const auto& s : candidates)
    {
        std::cout
            << std::left
            << std::setw(5) << i
            << std::setw(25) << sensors.sensorAt(s).chipName
            << std::setw(25) << sensors.sensorAt(s).label
            << std::setw(20) << sensorTypeToString(sensors.sensorAt(s).subfeatureType)
            << std::endl;
        i++;
    }

    std::cout << "\nSelect FAN-RPM Sensor: ";

    choice = {};

    std::cin >> choice;

    if (choice >= 0 && choice < candidates.size())
    {
        std::cout << "Selected: "
                  << sensors.sensorAt(candidates[choice]).chipName
                  << " "
                  << sensors.sensorAt(candidates[choice]).label
                  << "\n";

        parser.setString("sensor.rpm", "name", sensors.sensorAt(candidates[choice]).chipName);
        parser.setString("sensor.rpm", "label", sensors.sensorAt(candidates[choice]).label);
        parser.setString("sensor.rpm", "source", sensorSourceToString(sensors.sensorAt(candidates[choice]).source));
    }
    else
    {
        return 1;
    }

    // Map Sensor - FREQ
    candidates = sensors.findByType(SENSORS_SUBFEATURE_FREQ_INPUT);

    std::cout
        << std::left
        << std::setw(5) << "\nID"
        << std::setw(25) << "CHIP-NAME:"
        << std::setw(25) << "LABEL:"
        << std::setw(20) << "TYPE:\n"
        << std::endl;

    std::cout << std::left << std::string(75, '-') << '\n';

    i = 0;
    for (const auto& s : candidates)
    {
        std::cout
            << std::left
            << std::setw(5) << i
            << std::setw(25) << sensors.sensorAt(s).chipName
            << std::setw(25) << sensors.sensorAt(s).label
            << std::setw(20) << sensorTypeToString(sensors.sensorAt(s).subfeatureType)
            << std::endl;
        i++;
    }

    std::cout << "\nSelect FREQUENCY Sensor: ";

    choice = {};

    std::cin >> choice;

    if (choice >= 0 && choice < candidates.size())
    {

        std::cout << "Selected: "
                  << sensors.sensorAt(candidates[choice]).chipName
                  << " "
                  << sensors.sensorAt(candidates[choice]).label
                  << "\n";

        parser.setString("sensor.mhz", "name", sensors.sensorAt(candidates[choice]).chipName);
        parser.setString("sensor.mhz", "label", sensors.sensorAt(candidates[choice]).label);
        parser.setString("sensor.mhz", "source", sensorSourceToString(sensors.sensorAt(candidates[choice]).source));
    }
    else
    {
        return 1;
    }

    std::cout << "\nEnter time in sec for rotation, 0 = no rotation: ";

    choice = {};

    std::cin >> choice;

    if (choice == 0)
    {
        int subChoice;

        std::cout << "\n[0] Display always RPM."
                  << "\n[1] Display always TEMP."
                  << "\n[2] Display always FREQ."
                  << "\nNo Rotation selected. Please select static Layout: ";

        std::cin >> subChoice;
        if (subChoice < 0 || subChoice > 2)
        {
            return 1;
        }
        parser.setInt("settings", "staticLayout", subChoice);
    }

    parser.setInt("settings", "rotationInterval", choice);

    parser.writeToFile(options_.configPath.value());

    return EXIT_SUCCESS;
}
}