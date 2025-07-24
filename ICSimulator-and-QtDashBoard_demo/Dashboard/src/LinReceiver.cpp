#include "LinReceiver.h"

#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <cstring>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

// Constructor: Initializes LIN receiver for the specified device
LinReceiver::LinReceiver(QObject *parent) : QObject(parent), linFd(-1)
{
    // Open the LIN device file in read-only mode
    linFd = open("/dev/plin0", O_RDONLY);
    if (linFd < 0)
    {
        qFatal("Failed to open LIN device: %s", strerror(errno));
    }

    // Create a socket notifier to handle incoming LIN frames
    notifier = new QSocketNotifier(linFd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, &LinReceiver::readLinFrame);
    qDebug() << "LinReceiver Constructor is called";

    // Check if lin_protocol_receiver.json exists and is valid
    std::ifstream check_file("lin_protocol_receiver.json");
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
            catch (const nlohmann::json::exception &e)
            {
                qWarning() << "Existing lin_protocol_receiver.json is invalid:" << e.what();
                qWarning() << "File content:" << QString::fromStdString(content);
            }
        }
    }

    // Initialize lin_protocol_receiver.json with an empty array if invalid or missing
    if (needs_init)
    {
        std::ofstream json_file("lin_protocol_receiver.json");
        if (json_file.is_open())
        {
            json_file << "[]";
            json_file.close();
            qDebug() << "Initialized lin_protocol_receiver.json as empty array";
        }
        else
        {
            qWarning() << "Failed to initialize lin_protocol_receiver.json";
        }
    }
}

// Destructor: Cleans up resources
LinReceiver::~LinReceiver()
{
    // Close the LIN device file if it was opened
    if (linFd >= 0)
    {
        close(linFd);
    }
    // Delete the socket notifier
    delete notifier;
}

// Logs speed and RPM data to lin_protocol_receiver.json
void LinReceiver::logSignalToJson(float speed, int rpm) // Changed from float to int for rpm
{
    std::lock_guard<std::mutex> lock(m_mutex); // Ensure thread-safe access to the JSON file

    // Create a JSON entry for the signal data
    json signalEntry;
    signalEntry["Speed"] = speed; // Store speed in meters per second
    signalEntry["RPM"] = rpm;     // Store RPM as an integer

    // Read existing JSON data from file
    json entries = json::array(); // Default to empty array if file is invalid
    std::ifstream in_file("lin_protocol_receiver.json");
    std::string file_content;
    if (in_file.is_open())
    {
        std::stringstream buffer;
        buffer << in_file.rdbuf();
        file_content = buffer.str();
        in_file.close();

        if (!file_content.empty())
        {
            try
            {
                entries = json::parse(file_content);
                if (!entries.is_array())
                {
                    qWarning() << "lin_protocol_receiver.json is not an array. Resetting to empty array.";
                    entries = json::array();
                }
            }
            catch (const nlohmann::json::exception &e)
            {
                qWarning() << "Failed to parse lin_protocol_receiver.json:" << e.what();
                qWarning() << "File content:" << QString::fromStdString(file_content);
                entries = json::array();
            }
        }
    }
    else
    {
        qWarning() << "Failed to open lin_protocol_receiver.json for reading";
    }

    // Append the new signal entry to the JSON array
    entries.push_back(signalEntry);

    // Save the updated JSON array to the file
    saveJsonFile("lin_protocol_receiver.json", entries);
}
// Saves JSON data to a specified file
void LinReceiver::saveJsonFile(const std::string &filename, const nlohmann::json &json_data)
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

// Reads and processes incoming LIN frames
void LinReceiver::readLinFrame()
{
    struct plin_msg msg;
    // Read a LIN frame from the device
    ssize_t nbytes = read(linFd, &msg, sizeof(msg));
    if (nbytes < 0)
    {
        qWarning() << "Error reading LIN frame:" << strerror(errno);
        return;
    }
    // Check for incomplete frame (Note: 'buffer' is undefined, likely meant to be 'msg')
    if (nbytes != sizeof(msg)) {
        qWarning() << "Received incomplete LIN packet:" << nbytes << "bytes, expected" << sizeof(msg);
        return;
    }

    // Process LIN frame based on message type
    if (msg.type == PLIN_MSG_FRAME)
    {
        qDebug() << "Received LIN frame with ID:" << msg.id;

        // Handle frames with ID 0x04 (speed and RPM data)
        if (msg.id == 0x04)
        {
            // Convert frame data to a QByteArray (8 bytes)
            QByteArray data(reinterpret_cast<const char *>(msg.data), 8);
            QString dataStr = QString::fromLatin1(data);

            // Extract speed and RPM as strings from the ASCII data
            QString speedStr = dataStr.left(4); // First 4 characters for speed
            QString rpmStr = dataStr.mid(4, 4); // Last 4 characters for RPM

            // Convert strings to numerical values
            bool ok1 = false, ok2 = false;
            float speed_raw = speedStr.toFloat(&ok1);
            int rpm_raw = rpmStr.toInt(&ok2);

            if (ok1 && ok2)
            {
                // Convert speed from m/s to km/h
                float speed_converted = speed_raw * 3.6f;
                int rpm_converted = rpm_raw; // RPM is already in the correct unit

                // Log the raw and converted values
                qDebug().noquote() << QString("LIN Speed raw: %1, converted: %2 km/h; RPM: %3")
                                        .arg(speed_raw, 0, 'f', 2)
                                        .arg(speed_converted, 0, 'f', 2)
                                        .arg(rpm_raw);

                // Emit signals with converted values
                emit speedDataReceived(speed_converted);
                emit rpmDataReceived(rpm_converted);

                // Log raw values to JSON
                logSignalToJson(speed_raw, rpm_raw);
            }
            else
            {
                qWarning() << "Failed to convert ASCII LIN data to float/int.";
            }
        }
        // Add handlers for other message IDs (e.g., for fuel and temp) if needed
    }
    else if (msg.type == PLIN_MSG_OVERRUN)
    {
        qWarning() << "LIN message overrun detected!";
    }
    else if (msg.type == PLIN_MSG_WAKEUP)
    {
        qDebug() << "LIN wakeup message received!";
    }
    else
    {
        qWarning() << "Unsupported LIN message type:" << msg.type;
    }
}