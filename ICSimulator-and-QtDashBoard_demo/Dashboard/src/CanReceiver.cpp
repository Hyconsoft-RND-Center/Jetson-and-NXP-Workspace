#include "CanReceiver.h"
#include <QDebug>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <iostream>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>

using json = nlohmann::json;

// Constructor: Initializes CAN receiver with the specified interface
CanReceiver::CanReceiver(const QString &interfaceName, QObject *parent) : QObject(parent), socketFd(-1)
{
    // Create a raw CAN socket for communication
    socketFd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socketFd < 0)
    {
        qFatal("Failed to create CanReceiver socket");
    }

    // Configure the CAN interface using the provided interface name
    struct ifreq ifr;
    strncpy(ifr.ifr_name, interfaceName.toStdString().c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0'; // Ensure null-termination of interface name
    if (ioctl(socketFd, SIOCGIFINDEX, &ifr) < 0)
    {
        qFatal("Failed to retrieve interface index for %s", interfaceName.toStdString().c_str());
    }

    // Bind the socket to the specified CAN interface
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(socketFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        qFatal("Failed to bind CAN socket to %s", interfaceName.toStdString().c_str());
    }

    // Set up a socket notifier to handle incoming CAN frames
    notifier = new QSocketNotifier(socketFd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, &CanReceiver::readCanFrame);
    qDebug() << "CanReceiver initialized successfully for" << interfaceName;

    // Check if can_protocol_receiver.json exists and is valid
    std::ifstream check_file("can_protocol_receiver.json");
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
                qWarning() << "Existing can_protocol_receiver.json is invalid:" << e.what();
                qWarning() << "File content:" << QString::fromStdString(content);
            }
        }
    }

    // Initialize can_protocol_receiver.json with an empty array if invalid or missing
    if (needs_init)
    {
        std::ofstream json_file("can_protocol_receiver.json");
        if (json_file.is_open())
        {
            json_file << "[]";
            json_file.close();
            qDebug() << "Initialized can_protocol_receiver.json as empty array";
        }
        else
        {
            qWarning() << "Failed to initialize can_protocol_receiver.json";
        }
    }
}

// Destructor: Cleans up resources
CanReceiver::~CanReceiver()
{
    // Close the CAN socket if it was opened
    if (socketFd >= 0)
    {
        close(socketFd);
    }
    // Delete the socket notifier
    delete notifier;
}

// Saves JSON data to a specified file
void CanReceiver::saveJsonFile(const std::string &filename, const nlohmann::json &json_data)
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

// Logs speed and RPM data to can_protocol_receiver.json
void CanReceiver::logSignalToJson(float speed, int rpm) {
    std::lock_guard<std::mutex> lock(m_mutex); // Ensure thread-safe access to the JSON file
    // Create a JSON entry for the signal data
    json signalEntry;
    signalEntry["Speed"] = speed; // Store speed in meters per second
    signalEntry["RPM"] = rpm;     // Store RPM as an integer

    // Read existing JSON data from file
    json entries = json::array();
    std::ifstream in_file("can_protocol_receiver.json");
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
                    qWarning() << "can_protocol_receiver.json is not an array. Resetting to empty array.";
                    entries = json::array();
                }
            } catch (const json::exception &e) {
                qWarning() << "Failed to parse can_protocol_receiver.json:" << e.what();
                qWarning() << "File content:" << QString::fromStdString(file_content);
                entries = json::array();
            }
        }
    } else {
        qWarning() << "Failed to open can_protocol_receiver.json for reading";
    }

    // Append the new signal entry to the JSON array
    entries.push_back(signalEntry);
    // Save the updated JSON array to the file
    saveJsonFile("can_protocol_receiver.json", entries);
}

// Reads and processes incoming CAN frames
void CanReceiver::readCanFrame()
{
    struct can_frame frame;
    // Read a CAN frame from the socket
    ssize_t nbytes = read(socketFd, &frame, sizeof(struct can_frame));
    if (nbytes < 0)
    {
        qWarning() << "Error reading CAN frame";
        return;
    }
    if (nbytes < sizeof(struct can_frame))
    {
        qWarning() << "Short read, message truncated";
        return;
    }

    // Process frames with CAN ID 0x64
    if (frame.can_id == 0x64)
    {
        // Convert frame data to a QByteArray (8 bytes)
        QByteArray data(reinterpret_cast<const char *>(frame.data), 8);

        // Extract speed and RPM as strings from the ASCII data
        QString dataStr = QString::fromLatin1(data);
        QString speedStr = dataStr.left(4); // First 4 characters for speed
        QString rpmStr = dataStr.mid(4, 4); // Next 4 characters for RPM

        // Convert strings to numerical values
        bool ok1 = false, ok2 = false;
        float speed_raw = speedStr.toFloat(&ok1);
        int rpm_raw = rpmStr.toInt(&ok2);

        if (ok1 && ok2)
        {
            // Convert speed from m/s to km/h
            float speed_converted = speed_raw * 3.6f;
            float rpm_converted = rpm_raw; // RPM is already in the correct unit

            // Log the raw and converted values
            qDebug().noquote() << QString("CAN Speed raw: %1, converted: %2 km/h; RPM  %3")
                      .arg(speed_raw, 0, 'f', 2)
                      .arg(speed_converted, 0, 'f', 2)
                      .arg(rpm_raw, 0, 'f', 2);
                          
            // Emit signals with converted values
            emit speedDataReceived(speed_converted);
            emit rpmDataReceived(rpm_converted);
            
            // Log the raw data to JSON
            logSignalToJson(speed_raw, rpm_raw);
        }
        else
        {
            qWarning() << "Failed to convert ASCII CAN data to float.";
        }
    }
}