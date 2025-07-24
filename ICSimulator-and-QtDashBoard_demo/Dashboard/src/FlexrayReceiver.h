#ifndef FLEXRAYRECEIVER_H
#define FLEXRAYRECEIVER_H

#include <QObject>
#include <QSocketNotifier>
#include <QString>
#include <nlohmann/json.hpp>
#include <mutex>

// Class: FlexRayReceiver
// Description: Manages the reception and processing of FlexRay packets over UDP, parsing speed and RPM data,
//              and logging it to a JSON file. Inherits from QObject for signal-slot functionality.
class FlexRayReceiver : public QObject
{
    Q_OBJECT

public:
    // Constructor: Initializes the FlexRay receiver with the specified IP address and port.
    // Parameters:
    //   - ip: IP address to bind the UDP socket to.
    //   - port: Port number for UDP communication.
    //   - parent: Optional parent QObject for memory management.
    explicit FlexRayReceiver(const QString &ip, quint16 port, QObject *parent = nullptr);

    // Destructor: Cleans up resources, including closing the UDP socket and deleting the notifier.
    ~FlexRayReceiver();

signals:
    // Signal: Emitted when speed data is received from a FlexRay packet.
    // Parameters:
    //   - speed: Vehicle speed in km/h.
    void speedDataReceived(float speed);

    // Signal: Emitted when RPM data is received from a FlexRay packet.
    // Parameters:
    //   - rpm: Engine RPM as a float.
    void rpmDataReceived(float rpm);

    // Signal: Emitted when fuel data is received from a FlexRay packet.
    // Parameters:
    //   - fuel: Fuel level or consumption data.
    void fuelDataReceived(float fuel);

    // Signal: Emitted when temperature data is received from a FlexRay packet.
    // Parameters:
    //   - temp: Temperature reading (e.g., engine or coolant temperature).
    void tempDataReceived(float temp);

private slots:
    // Slot: Reads and processes incoming FlexRay packets from the UDP socket.
    void readflexrayPacket();

private:
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

    // Member: File descriptor for the UDP socket.
    int socketFd;

    // Member: QSocketNotifier for asynchronous monitoring of UDP socket events.
    QSocketNotifier *notifier;

    // Member: Mutex for thread-safe access to the JSON file.
    std::mutex m_mutex;
};

#endif // FLEXRAYRECEIVER_H