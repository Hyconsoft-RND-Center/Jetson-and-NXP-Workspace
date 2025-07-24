#pragma once
#include <QObject>
#include <QSocketNotifier>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include <mutex>

// Class: CanReceiver
// Description: Manages the reception and processing of CAN bus frames, parsing speed and RPM data,
//              and logging it to a JSON file. Inherits from QObject for signal-slot functionality.
class CanReceiver : public QObject
{
    Q_OBJECT
public:
    // Constructor: Initializes the CAN receiver with the specified interface name.
    // Parameters:
    //   - interfaceName: The name of the CAN interface (e.g., "can0").
    //   - parent: Optional parent QObject for memory management.
    explicit CanReceiver(const QString &interfaceName, QObject *parent = nullptr);

    // Destructor: Cleans up resources, including closing the CAN socket and deleting the notifier.
    ~CanReceiver();

signals:
    // Signal: Emitted when speed data is received from a CAN frame.
    // Parameters:
    //   - speed: Vehicle speed in km/h.
    void speedDataReceived(float speed);

    // Signal: Emitted when RPM data is received from a CAN frame.
    // Parameters:
    //   - rpm: Engine RPM as a float.
    void rpmDataReceived(float rpm);

    // Signal: Emitted when fuel data is received from a CAN frame.
    // Parameters:
    //   - fuel: Fuel level or consumption data.
    void fuelDataReceived(float fuel);

    // Signal: Emitted when temperature data is received from a CAN frame.
    // Parameters:
    //   - temp: Temperature reading (e.g., engine or coolant temperature).
    void tempDataReceived(float temp);

private slots:
    // Slot: Reads and processes incoming CAN frames from the socket.
    void readCanFrame();

private:
    // Member: File descriptor for the CAN socket.
    int socketFd;

    // Member: QSocketNotifier for asynchronous monitoring of CAN socket events.
    QSocketNotifier *notifier;

    // Function: Logs speed and RPM data to a JSON file.
    // Parameters:
    //   - speed: Raw speed value in meters per second.
    //   - rpm: Raw RPM value as an integer.
    void logSignalToJson(float speed, int rpm);

    // Function: Saves JSON data to the specified file.
    // Parameters:
    //   - filename: Path to the JSON file.
    //   - json_data: JSON data to be written.
    void saveJsonFile(const std::string &filename, const nlohmann::json &json_data);

    // Member: Mutex for thread-safe access to the JSON file.
    std::mutex m_mutex;
};