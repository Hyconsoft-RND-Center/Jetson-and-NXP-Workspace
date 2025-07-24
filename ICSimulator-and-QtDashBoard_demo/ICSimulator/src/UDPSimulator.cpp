#include "UDPSimulator.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <functional>
#include <arpa/inet.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Constructor: Initializes UDP socket and JSON logging
UDPSimulator::UDPSimulator(const std::string& ip, int port) : m_socket(-1)
{
    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket < 0) {
        perror("Failed to create UDP socket");
        return;
    }

    memset(&m_serverAddr, 0, sizeof(m_serverAddr));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip.c_str(), &m_serverAddr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        closeSocket();
        return;
    }

    // Open JSON file for writing (truncate to overwrite)
    protocol_sender_json.open("udp_protocol_sender_json.json", std::ios::out | std::ios::trunc);
    if (!protocol_sender_json.is_open()) {
        std::cerr << "Failed to open udp_protocol_sender_json.json for writing\n";
    }
}

// Destructor: Closes socket and JSON file
UDPSimulator::~UDPSimulator()
{
    closeSocket();
    if (protocol_sender_json.is_open()) {
        protocol_sender_json.close();
    }
}

// Closes the UDP socket if open
void UDPSimulator::closeSocket()
{
    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
    }
}

// Logs signal data to JSON file
template<typename T>
void UDPSimulator::logSignalToJson(const std::string& signalName, const T& value)
{
    json signalEntry;
    signalEntry[signalName] = value;

    if (protocol_sender_json.is_open()) {
        protocol_sender_json << signalEntry.dump() << std::endl;
        protocol_sender_json.flush();
    }
}

 // Sends speed and RPM data in a single UDP packet
bool UDPSimulator::sendUDPData(float speed, float rpm)
{
        if (m_socket < 0) {
            std::cerr << "Socket is not open!" << std::endl;
            return false;
        }
        //Ensure non-negative values
        speed = std::max(0.0f, speed);
        rpm = std::max(0.0f, rpm);

        // Pack data into an 8-byte buffer (4 bytes for speed, 4 bytes for rpm)
        uint8_t buffer[8];
        memcpy(buffer, &speed, sizeof(float));     // Bytes 0-3: speed
        memcpy(buffer + 4, &rpm, sizeof(float));   // Bytes 4-7: rpm

        // Log each signal individually
        logSignalToJson("Speed", speed);
        logSignalToJson("RPM", rpm);

        // Debug print to verify sent values
        std::cout << "Sending: Speed=" << speed << ", RPM=" << rpm << std::endl;

        // Send the buffer
         ssize_t sent = sendto(m_socket, buffer, sizeof(buffer), 0,
                         (struct sockaddr*)&m_serverAddr, 
                         sizeof(m_serverAddr));


        if (sent < 0) {
            std::cerr << "Failed to send UDP message: " << strerror(errno) << std::endl;
            return false;
        }

        return true;
}

// Simulates float data from start to end, with optional reverse direction
void simulateFloatData(const std::function<void(float, float)>& sendData, float startSpeed, float endSpeed, float startRPM, float endRPM, float stepSpeed, float stepRPM, std::chrono::milliseconds delay, bool reverse = false)
{
    if (startSpeed < 0 || endSpeed < 0 || startRPM < 0 || endRPM < 0 || stepSpeed <= 0 || stepRPM <= 0)
    {
        std::cerr << "Invalid simulation parameters (negative or zero)\n";
        return;
    }

    if (!reverse)
    {
        // Forward: start to end
        for (float speed = startSpeed, rpm = startRPM; speed <= endSpeed && rpm <= endRPM; speed += stepSpeed, rpm += stepRPM)
        {
            sendData(std::min(speed, endSpeed), std::min(rpm, endRPM));
            std::this_thread::sleep_for(delay);
        }
    }
    else
    {
        // Reverse: end to start
        for (float speed = endSpeed, rpm = endRPM; speed >= startSpeed && rpm >= startRPM; speed -= stepSpeed, rpm -= stepRPM)
        {
            sendData(std::max(startSpeed, speed), std::max(startRPM, rpm));
            std::this_thread::sleep_for(delay);
        }
    }
}

// Main: Runs simulation loops for combined signals
int main()
{
    UDPSimulator udpSimulator("127.0.0.1", 5000);

    // Calculate proper RPM step to reach 2223 when speed reaches 73
    const float max_speed = 78.0f;
    const float max_rpm = 2223.0f;
    const float speed_step = 5.0f;
    const float rpm_step = (max_rpm / (max_speed / speed_step));

    while (true) {
        // Simulate speed (0 to 73 km/h) and RPM (0 to 2223) with synchronized steps
        simulateFloatData(
            [&udpSimulator](float speed, float rpm) { 
                udpSimulator.sendUDPData(speed, rpm); 
            },
            0.0f, max_speed, 0.0f, max_rpm, speed_step, rpm_step, std::chrono::milliseconds(100)
        );
        simulateFloatData(
            [&udpSimulator](float speed, float rpm) { 
                udpSimulator.sendUDPData(speed, rpm); 
            },
            0.0f, max_speed, 0.0f, max_rpm, speed_step, rpm_step, std::chrono::milliseconds(100), true
        );
    }

    return 0;
}