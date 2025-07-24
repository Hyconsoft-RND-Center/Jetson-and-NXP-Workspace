#include "ICSimulator.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <functional>
#include <chrono>
#include <thread>
using json = nlohmann::json;

// Constructor
ICSimulator::ICSimulator() : m_socket(socket(PF_CAN, SOCK_RAW, CAN_RAW))
{
    if (m_socket < 0)
    {
        perror("Failed to create ICSimulator socket");
        return;
    }

    fcntl(m_socket, F_SETFL, O_NONBLOCK);

    struct ifreq ifr;
    strncpy(ifr.ifr_name, "vcan0", IFNAMSIZ);
    if (ioctl(m_socket, SIOCGIFINDEX, &ifr) < 0)
    {
        perror("Failed to retrieve interface index");
        closeSocket();
        return;
    }

    struct sockaddr_can addr = {};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(m_socket, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        perror("Failed to bind ICSimulator socket");
        closeSocket();
    }

    protocol_sender_json.open("can_protocol_sender_json.json", std::ios::out | std::ios::trunc);
    if (!protocol_sender_json.is_open())
    {
        std::cerr << "Failed to open can_protocol_sender_json.json for writing\n";
    }
}

// Destructor
ICSimulator::~ICSimulator()
{
    closeSocket();
    if (protocol_sender_json.is_open())
        protocol_sender_json.close();
}

// Close socket
void ICSimulator::closeSocket()
{
    if (m_socket >= 0)
    {
        close(m_socket);
        m_socket = -1;
    }
}

// Send CAN data
bool ICSimulator::sendCANData(uint32_t canId, const void* data, size_t dataSize)
{
    if (m_socket < 0)
    {
        std::cerr << "Socket is not open!\n";
        return false;
    }

    struct can_frame frame = {};
    frame.can_id = canId;
    frame.can_dlc = 8;
    memset(frame.data, 0, sizeof(frame.data));
    memcpy(frame.data, data, dataSize);

    if (write(m_socket, &frame, sizeof(frame)) != sizeof(frame))
    {
        perror("Failed to send CAN message");
        return false;
    }
    return true;
}

// Log signal to JSON
template<typename T>
void ICSimulator::logSignalToJson(const std::string& signalName, const T& value)
{
    json signalEntry;
    signalEntry[signalName] = value;

    if (protocol_sender_json.is_open())
    {
        protocol_sender_json << signalEntry.dump() << std::endl;
        protocol_sender_json.flush();
    }
}

// Send speed and rpm
bool ICSimulator::sendCombinedData(float speed, float rpm)
{
    speed = std::max(0.0f, speed);
    rpm = std::max(0.0f, rpm);

    std::cout << "SPEED = " << speed << ", RPM = " << rpm << std::endl;

    logSignalToJson("Speed", speed);
    logSignalToJson("RPM", rpm);

    uint8_t buffer[8];
    memcpy(buffer, &speed, sizeof(float));
    memcpy(buffer + 4, &rpm, sizeof(float));

    return sendCANData(0x64, buffer, sizeof(buffer));
}

// Simulates float data from start to end, with optional reverse direction
void simulateFloatData(const std::function<void(float, float)>& sendData, 
                     float startSpeed, float endSpeed, 
                     float startRPM, float endRPM, 
                     float stepSpeed, float stepRPM, 
                     std::chrono::milliseconds delay, 
                     bool reverse = false)
{
    if (startSpeed < 0 || endSpeed < 0 || startRPM < 0 || endRPM < 0 || stepSpeed <= 0 || stepRPM <= 0)
    {
        std::cerr << "Invalid simulation parameters (negative or zero)\n";
        return;
    }

    if (!reverse)
    {
        // Forward: start to end
        for (float speed = startSpeed, rpm = startRPM; 
             speed <= endSpeed && rpm <= endRPM; 
             speed += stepSpeed, rpm += stepRPM)
        {
            sendData(std::min(speed, endSpeed), std::min(rpm, endRPM));
            std::this_thread::sleep_for(delay);
        }
    }
    else
    {
        // Reverse: end to start
        for (float speed = endSpeed, rpm = endRPM; 
             speed >= startSpeed && rpm >= startRPM; 
             speed -= stepSpeed, rpm -= stepRPM)
        {
            sendData(std::max(startSpeed, speed), std::max(startRPM, rpm));
            std::this_thread::sleep_for(delay);
        }
    }
}

// Main: Runs simulation loops for combined signals
int main()
{
    ICSimulator icSimulator;

    // Calculate proper RPM step to reach 2223 when speed reaches 73
    const float max_speed = 78.0f;
    const float max_rpm = 2223.0f;
    const float speed_step = 5.0f;
    const float rpm_step = (max_rpm / (max_speed / speed_step));

    while (true) {
        // Simulate speed (0 to 73 km/h) and RPM (0 to 2223) with synchronized steps
        simulateFloatData(
            [&icSimulator](float speed, float rpm) { 
                icSimulator.sendCombinedData(speed, rpm); 
            },
            0.0f, max_speed, 0.0f, max_rpm, speed_step, rpm_step, std::chrono::milliseconds(100)
        );
        simulateFloatData(
            [&icSimulator](float speed, float rpm) { 
                icSimulator.sendCombinedData(speed, rpm); 
            },
            0.0f, max_speed, 0.0f, max_rpm, speed_step, rpm_step, std::chrono::milliseconds(100), true
        );

        // Below implementations can be used in future, hence commented
        // // Simulate fuel (0 to 1, step 0.05)
        // simulateFloatData(
        //     [&icSimulator](float fuel, float /*unused*/) { 
        //         icSimulator.sendCombinedData(0.0f, 0.0f); 
        //     },
        //     0.0f, 1.0f, 0.0f, 0.0f, 0.05f, 0.0f, std::chrono::milliseconds(100)
        // );
        // simulateFloatData(
        //     [&icSimulator](float fuel, float /*unused*/) { 
        //         icSimulator.sendCombinedData(0.0f, 0.0f); 
        //     },
        //     0.0f, 1.0f, 0.0f, 0.0f, 0.05f, 0.0f, std::chrono::milliseconds(100), true
        // );

        // // Simulate temperature (-40 to 125°C, step 5)
        // simulateFloatData(
        //     [&icSimulator](float temp, float /*unused*/) { 
        //         icSimulator.sendCombinedData(0.0f, 0.0f); 
        //     },
        //     -40.0f, 125.0f, 0.0f, 0.0f, 5.0f, 0.0f, std::chrono::milliseconds(100)
        // );
        // simulateFloatData(
        //     [&icSimulator](float temp, float /*unused*/) { 
        //         icSimulator.sendCombinedData(0.0f, 0.0f); 
        //     },
        //     -40.0f, 125.0f, 0.0f, 0.0f, 5.0f, 0.0f, std::chrono::milliseconds(100), true
        // );
    }

    return 0;
}