#ifndef LINRECEIVER_H
#define LINRECEIVER_H

#include <QObject>
#include <QSocketNotifier>
#include <nlohmann/json.hpp>
#include <mutex>
#include "plin.h"

// Class: LinReceiver
// Description: Manages the reception and processing of LIN bus frames, parsing speed and RPM data,
//              and logging it to a JSON file. Inherits from QObject for signal-slot functionality.
class LinReceiver : public QObject
{
    Q_OBJECT

public:
    // Constructor: Initializes the LIN receiver for the specified device.
    // Parameters:
    //   - parent: Optional parent QObject for memory management.
    explicit LinReceiver(QObject *parent = nullptr);

    // Destructor: Cleans up resources, including closing the LIN device file and deleting the notifier.
    ~LinReceiver();

signals:
    // Signal: Emitted when speed data is received from a LIN frame.
    // Parameters:
    //   - speed: Vehicle speed in km/h.
    void speedDataReceived(float speed);

    // Signal: Emitted when RPM data is received from a LIN frame.
    // Parameters:
    //   - rpm: Engine RPM as a float.
    void rpmDataReceived(float rpm);

    // Signal: Emitted when fuel data is received from a LIN frame.
    // Parameters:
    //   - fuel: Fuel level or consumption data.
    void fuelDataReceived(float fuel);

    // Signal: Emitted when temperature data is received from a LIN frame.
    // Parameters:
    //   - temp: Temperature reading (e.g., engine or coolant temperature).
    void tempDataReceived(float temp);

private slots:
    // Slot: Reads and processes incoming LIN frames from the device.
    void readLinFrame();

private:
    // Member: File descriptor for the LIN device.
    int linFd;

    // Member: QSocketNotifier for asynchronous monitoring of LIN device events.
    QSocketNotifier *notifier;

    // Member: Mutex for thread-safe access to the JSON file.
    std::mutex m_mutex;

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
};

#endif // LINRECEIVER_H