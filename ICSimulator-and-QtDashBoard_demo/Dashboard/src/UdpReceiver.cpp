#include "UdpReceiver.h"
#include <QDebug>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

// Constructor: Initializes UDP receiver with the specified IP and port
UdpReceiver::UdpReceiver(const QString &ip, quint16 port, QObject *parent)
    : QObject(parent), socketFd(-1)
{
    // Create a UDP socket for communication
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd < 0)
    {
        qFatal("Failed to create UDP socket");
    }

    // Configure server address for binding
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); // Convert port to network byte order
    if (inet_pton(AF_INET, ip.toStdString().c_str(), &serverAddr.sin_addr) <= 0)
    {
        qFatal("Invalid IP address: %s", ip.toStdString().c_str());
    }

    // Bind the socket to the specified IP and port
    if (bind(socketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        qFatal("Failed to bind UDP socket to %s:%d", ip.toStdString().c_str(), port);
    }

    // Set up a socket notifier to handle incoming UDP packets
    notifier = new QSocketNotifier(socketFd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, &UdpReceiver::readUdpPacket);
    qDebug() << "UdpReceiver initialized successfully for" << ip << ":" << port;

    // Check if udp_protocol_receiver.json exists and is valid
    std::ifstream check_file("udp_protocol_receiver.json");
    bool needs_init = true;
    if (check_file.is_open())
    {
        // Read the content of the JSON file
        std::stringstream buffer;
        buffer << check_file.rdbuf();
        std::string content = buffer.str();
        check_file.close();

        if (!content.empty())
        {
            try
            {
                json temp = json::parse(content);
                if (temp.is_array())
                {
                    needs_init = false; // Valid JSON array found, no initialization needed
                }
            }
            catch (const json::exception &e)
            {
                qWarning() << "Existing udp_protocol_receiver.json is invalid:" << e.what();
                qWarning() << "File content:" << QString::fromStdString(content);
            }
        }
    }

    // Initialize udp_protocol_receiver.json with an empty array if invalid or missing
    if (needs_init)
    {
        std::ofstream json_file("udp_protocol_receiver.json");
        if (json_file.is_open())
        {
            json_file << "[]";
            json_file.close();
            qDebug() << "Initialized udp_protocol_receiver.json as empty array";
        }
        else
        {
            qWarning() << "Failed to initialize udp_protocol_receiver.json";
        }
    }
}

// Destructor: Cleans up resources
UdpReceiver::~UdpReceiver()
{
    // Close the UDP socket if it was opened
    if (socketFd >= 0)
    {
        close(socketFd);
    }
    // Delete the socket notifier
    delete notifier;
}

// Saves JSON data to a specified file
void UdpReceiver::saveJsonFile(const std::string &filename, const nlohmann::json &json_data)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        qWarning() << "Failed to save JSON file:" << QString::fromStdString(filename);
        return;
    }
 
    // Write JSON data with 2-space indentation
    file << json_data.dump(2);
    file.close();
    qDebug() << "Saved JSON file:" << QString::fromStdString(filename);
}

// Logs speed and RPM data to udp_protocol_receiver.json
void UdpReceiver::logSignalToJson(float speed, int rpm) {
    std::lock_guard<std::mutex> lock(m_mutex); // Ensure thread-safe access to the JSON file
    // Create a JSON entry for the signal data
    json signalEntry;
    signalEntry["Speed"] = speed; // Store speed in meters per second
    signalEntry["RPM"] = rpm;     // Store RPM as an integer

    // Read existing JSON data from file
    json entries = json::array();
    std::ifstream in_file("udp_protocol_receiver.json");
    std::string file_content;
    if (in_file.is_open()) {
        std::stringstream buffer;
        buffer << in_file.rdbuf();
        file_content = buffer.str();
        in_file.close();
        if (!file_content.empty()) {
            try {
                entries = json::parse(file_content);
                if (!entries.is_array()) {
                    qWarning() << "udp_protocol_receiver.json is not an array. Resetting to empty array.";
                    entries = json::array();
                }
            } catch (const json::exception &e) {
                qWarning() << "Failed to parse udp_protocol_receiver.json:" << e.what();
                qWarning() << "File content:" << QString::fromStdString(file_content);
                entries = json::array();
            }
        }
    } else {
        qWarning() << "Failed to open udp_protocol_receiver.json for reading";
    }

    // Append the new signal entry to the JSON array
    entries.push_back(signalEntry);
    // Save the updated JSON array to the file
    saveJsonFile("udp_protocol_receiver.json", entries);
}

// Reads and processes incoming UDP packets
void UdpReceiver::readUdpPacket() {
    uint8_t buffer[8];
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    // Receive a UDP packet
    ssize_t nbytes = recvfrom(socketFd, buffer, sizeof(buffer), 0,
                              (struct sockaddr *)&clientAddr, &clientLen);
    if (nbytes < 0) {
        qWarning() << "Error reading UDP packet:" << strerror(errno);
        return;
    }
    if (nbytes != sizeof(buffer)) {
        qWarning() << "Received incomplete UDP packet:" << nbytes << "bytes, expected" << sizeof(buffer);
        return;
    }

    // Convert packet data to a QByteArray (8 bytes)
    QByteArray data(reinterpret_cast<const char *>(buffer), 8);
    // Extract speed and RPM as strings from the ASCII data
    QString dataStr = QString::fromLatin1(data);
    QString speedStr = dataStr.left(4); // First 4 characters for speed
    QString rpmStr = dataStr.mid(4, 4); // Last 4 characters for RPM

    // Convert strings to numerical values
    bool ok1 = false, ok2 = false;
    float speed_raw = speedStr.toFloat(&ok1);
    int rpm_raw = rpmStr.toInt(&ok2);

    if (ok1 && ok2) {
        // Convert speed from m/s to km/h
        float speed_converted = speed_raw * 3.6f;
        int rpm_converted = rpm_raw; // RPM is already in the correct unit

        // Log the raw and converted values
        qDebug().noquote() << QString("UDP Speed raw: %1, converted: %2 km/h; RPM: %3")
                                .arg(speed_raw, 0, 'f', 2)
                                .arg(speed_converted, 0, 'f', 2)
                                .arg(rpm_raw);

        // Emit signals with converted values
        emit speedDataReceived(speed_converted);
        emit rpmDataReceived(rpm_converted);

        // Log raw values to JSON
        logSignalToJson(speed_raw, rpm_raw);
    } else {
        qWarning() << "Failed to convert ASCII UDP data to float/int.";
    }
}