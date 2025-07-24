#include <iostream>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <functional>
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>

using json = nlohmann::json;

class UDPSimulator {
private:
    int m_socket;
    struct sockaddr_in m_serverAddr;
    std::mutex m_mutex;

public:
    UDPSimulator(const std::string& ip, int port) : m_socket(-1) {
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
    }

    ~UDPSimulator() {
        closeSocket();
    }

    void closeSocket() {
        if (m_socket >= 0) {
            close(m_socket);
            m_socket = -1;
        }
    }

    void logToJson(float speed, float rpm) {
        std::lock_guard<std::mutex> lock(m_mutex);
        json signalEntry;
        signalEntry["Speed"] = speed;
        signalEntry["RPM"] = rpm;

        json entries = json::array();
        std::ifstream in_file("original_sender.json");
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
                        std::cerr << "original_sender.json is not an array. Resetting to empty array." << std::endl;
                        entries = json::array();
                    }
                } catch (const json::exception &e) {
                    std::cerr << "Failed to parse original_sender.json: " << e.what() << std::endl;
                    std::cerr << "File content: " << file_content << std::endl;
                    entries = json::array();
                }
            }
        } else {
            std::cerr << "Failed to open original_sender.json for reading" << std::endl;
        }

        entries.push_back(signalEntry);
        saveJsonFile("original_sender.json", entries);
    }

    void saveJsonFile(const std::string &filename, const json &json_data) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to save JSON file: " << filename << std::endl;
            return;
        }

        file << json_data.dump(2);
        file.close();
        std::cout << "Saved JSON file: " << filename << std::endl;
    }

    bool sendUDPData(float speed, float rpm) {
        if (m_socket < 0) {
            std::cerr << "UDP Socket is not open!" << std::endl;
            return false;
        }

        speed = std::max(0.0f, speed);
        rpm = std::max(0.0f, rpm);

        std::ostringstream speedStream, rpmStream;
        speedStream << std::fixed << std::setprecision(1) << speed;
        rpmStream << std::fixed << std::setprecision(0) << rpm;

        std::string speedStr = speedStream.str();
        std::string rpmStr = rpmStream.str();

        speedStr = (speedStr.size() > 4 ? speedStr.substr(0, 4) : speedStr + std::string(4 - speedStr.size(), ' '));
        rpmStr = (rpmStr.size() > 4 ? rpmStr.substr(0, 4) : rpmStr + std::string(4 - rpmStr.size(), ' '));

        uint8_t buffer[8];
        memcpy(buffer, speedStr.c_str(), 4);
        memcpy(buffer + 4, rpmStr.c_str(), 4);

        logToJson(speed, rpm);
        std::cout << "UDP Sending: Speed=" << speed << ", RPM=" << rpm << std::endl;

        ssize_t sent = sendto(m_socket, buffer, sizeof(buffer), 0,
                              (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr));

        if (sent < 0) {
            std::cerr << "Failed to send UDP message: " << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }
};

class ICSimulator {
private:
    int m_socket;
    std::mutex m_mutex;

public:
    ICSimulator() : m_socket(socket(PF_CAN, SOCK_RAW, CAN_RAW)) {
        if (m_socket < 0) {
            perror("Failed to create CAN socket");
            return;
        }

        fcntl(m_socket, F_SETFL, O_NONBLOCK);

        struct ifreq ifr;
        strncpy(ifr.ifr_name, "vcan0", IFNAMSIZ);
        if (ioctl(m_socket, SIOCGIFINDEX, &ifr) < 0) {
            perror("Failed to retrieve CAN interface index");
            closeSocket();
            return;
        }

        struct sockaddr_can addr = {};
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;
        if (bind(m_socket, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            perror("Failed to bind CAN socket");
            closeSocket();
        }
    }

    ~ICSimulator() {
        closeSocket();
    }

    void closeSocket() {
        if (m_socket >= 0) {
            close(m_socket);
            m_socket = -1;
        }
    }

    void logToJson(float speed, float rpm) {
        std::lock_guard<std::mutex> lock(m_mutex);
        json signalEntry;
        signalEntry["Speed"] = speed;
        signalEntry["RPM"] = rpm;

        json entries = json::array();
        std::ifstream in_file("original_sender.json");
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
                        std::cerr << "original_sender.json is not an array. Resetting to empty array." << std::endl;
                        entries = json::array();
                    }
                } catch (const json::exception &e) {
                    std::cerr << "Failed to parse original_sender.json: " << e.what() << std::endl;
                    std::cerr << "File content: " << file_content << std::endl;
                    entries = json::array();
                }
            }
        } else {
            std::cerr << "Failed to open original_sender.json for reading" << std::endl;
        }

        entries.push_back(signalEntry);
        saveJsonFile("original_sender.json", entries);
    }

    void saveJsonFile(const std::string &filename, const json &json_data) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to save JSON file: " << filename << std::endl;
            return;
        }

        file << json_data.dump(2);
        file.close();
        std::cout << "Saved JSON file: " << filename << std::endl;
    }

    bool sendCANData(uint32_t canId, const void *data, size_t dataSize) {
        if (m_socket < 0) {
            std::cerr << "CAN Socket is not open!" << std::endl;
            return false;
        }

        struct can_frame frame = {};
        frame.can_id = canId;
        frame.can_dlc = 8;
        memset(frame.data, 0, sizeof(frame.data));
        memcpy(frame.data, data, dataSize);

        if (write(m_socket, &frame, sizeof(frame)) != sizeof(frame)) {
            perror("Failed to send CAN message");
            return false;
        }
        return true;
    }

    bool sendCombinedData(float speed, float rpm) {
        speed = std::max(0.0f, speed);
        rpm = std::max(0.0f, rpm);

        std::ostringstream speedStream, rpmStream;
        speedStream << std::fixed << std::setprecision(1) << speed;
        rpmStream << std::fixed << std::setprecision(0) << rpm;

        std::string speedStr = speedStream.str();
        std::string rpmStr = rpmStream.str();

        speedStr = (speedStr.size() > 4 ? speedStr.substr(0, 4) : speedStr + std::string(4 - speedStr.size(), ' '));
        rpmStr = (rpmStr.size() > 4 ? rpmStr.substr(0, 4) : rpmStr + std::string(4 - rpmStr.size(), ' '));

        uint8_t buffer[8];
        memcpy(buffer, speedStr.c_str(), 4);
        memcpy(buffer + 4, rpmStr.c_str(), 4);

        // logToJson(speed, rpm);
        std::cout << "CAN Sending: Speed=" << speed << ", RPM=" << rpm << std::endl;

        return sendCANData(0x64, buffer, sizeof(buffer));
    }
};

class FlexRaySimulator {
private:
    int socketFd;
    struct sockaddr_in serverAddr;
    std::mutex m_mutex;

public:
    FlexRaySimulator() : socketFd(-1) {
        socketFd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socketFd < 0) {
            perror("Failed to create FlexRay UDP socket");
            return;
        }

        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(5002);
        if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
            perror("Invalid address for FlexRay UDP");
            closeSocket();
            return;
        }
    }

    ~FlexRaySimulator() { closeSocket(); }

    void closeSocket() {
        if (socketFd >= 0) {
            close(socketFd);
            socketFd = -1;
        }
    }

    void logToJson(float speed, float rpm) {
        std::lock_guard<std::mutex> lock(m_mutex);
        json signalEntry;
        signalEntry["Speed"] = speed;
        signalEntry["RPM"] = rpm;

        json entries = json::array();
        std::ifstream in_file("original_sender.json");
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
                        std::cerr << "original_sender.json is not an array. Resetting to empty array." << std::endl;
                        entries = json::array();
                    }
                } catch (const json::exception &e) {
                    std::cerr << "Failed to parse original_sender.json: " << e.what() << std::endl;
                    entries = json::array();
                }
            }
        }

        entries.push_back(signalEntry);
        saveJsonFile("original_sender.json", entries);
    }

    void saveJsonFile(const std::string &filename, const json &json_data) {
        std::ofstream file(filename);
        if (file.is_open()) {
            file << json_data.dump(2);
            file.close();
            std::cout << "Saved JSON file: " << filename << std::endl;
        }
    }

    bool sendFlexRayData(uint32_t slotId, const void *data, size_t dataSize) {
        if (socketFd < 0) {
            std::cerr << "FlexRay UDP Socket is not open!" << std::endl;
            return false;
        }

        uint8_t buffer[12]; // 4 bytes slotId + 8 bytes data
        uint32_t netSlotId = htonl(slotId);
        memcpy(buffer, &netSlotId, sizeof(netSlotId));
        memcpy(buffer + 4, data, dataSize);

        ssize_t sent = sendto(socketFd, buffer, sizeof(buffer), 0,
                              (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        if (sent < 0) {
            std::cerr << "Failed to send FlexRay UDP message: " << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }

    bool sendCombinedData(float speed, float rpm) {
        speed = std::max(0.0f, speed);
        rpm = std::max(0.0f, rpm);

        std::ostringstream speedStream, rpmStream;
        speedStream << std::fixed << std::setprecision(1) << speed;
        rpmStream << std::fixed << std::setprecision(0) << rpm;

        std::string speedStr = speedStream.str();
        std::string rpmStr = rpmStream.str();

        speedStr = (speedStr.size() > 4 ? speedStr.substr(0, 4) : speedStr + std::string(4 - speedStr.size(), ' '));
        rpmStr = (rpmStr.size() > 4 ? rpmStr.substr(0, 4) : rpmStr + std::string(4 - rpmStr.size(), ' '));

        uint8_t buffer[8];
        memcpy(buffer, speedStr.c_str(), 4);
        memcpy(buffer + 4, rpmStr.c_str(), 4);

        // logToJson(speed, rpm);
        std::cout << "FlexRay Sending: Speed=" << speed << ", RPM=" << rpm << std::endl;

        return sendFlexRayData(1, buffer, sizeof(buffer));
    }
};

void simulateFloatData(const std::function<void(float, float)> &sendData,
                       float startSpeed, float endSpeed,
                       float startRPM, float endRPM,
                       float stepSpeed, float stepRPM,
                       std::chrono::milliseconds delay) {
    if (startSpeed < 0 || endSpeed < 0 || startRPM < 0 || endRPM < 0 || stepSpeed <= 0 || stepRPM <= 0) {
        std::cerr << "Invalid simulation parameters (negative or zero)" << std::endl;
        return;
    }

    size_t speedSteps = static_cast<size_t>((endSpeed - startSpeed) / stepSpeed) + 1;
    size_t rpmSteps = static_cast<size_t>((endRPM - startRPM) / stepRPM) + 1;
    size_t speedCycleSteps = 2 * speedSteps - 1;
    size_t rpmCycleSteps = 2 * rpmSteps - 1;

    float speed = startSpeed;
    float rpm = startRPM;
    bool speedIncreasing = true;
    bool rpmIncreasing = true;
    size_t speedIndex = 0;
    size_t rpmIndex = 0;

    while (true) {
        if (speedIncreasing) {
            speed = startSpeed + speedIndex * stepSpeed;
            speed = std::min(speed, endSpeed);
            speedIndex++;
            if (speed >= endSpeed) {
                speedIncreasing = false;
            }
        } else {
            speed = endSpeed - (speedIndex - speedSteps) * stepSpeed;
            speed = std::max(startSpeed, speed);
            speedIndex++;
            if (speed <= startSpeed) {
                speedIncreasing = true;
                speedIndex = 0;
            }
        }

        if (rpmIncreasing) {
            rpm = startRPM + rpmIndex * stepRPM;
            rpm = std::min(rpm, endRPM);
            rpmIndex++;
            if (rpm >= endRPM) {
                rpmIncreasing = false;
            }
        } else {
            rpm = endRPM - (rpmIndex - rpmSteps) * stepRPM;
            rpm = std::max(startRPM, rpm);
            rpmIndex++;
            if (rpm <= startRPM) {
                rpmIncreasing = true;
                rpmIndex = 0;
            }
        }

        sendData(speed, rpm);
        std::this_thread::sleep_for(delay);
    }
}

int main() {
    std::ifstream check_file("original_sender.json");
    bool needs_init = true;
    if (check_file.is_open()) {
        std::stringstream buffer;
        buffer << check_file.rdbuf();
        std::string content = buffer.str();
        check_file.close();

        if (!content.empty()) {
            try {
                json temp = json::parse(content);
                if (temp.is_array()) {
                    needs_init = false;
                }
            } catch (const json::exception& e) {
                std::cerr << "Existing original_sender.json is invalid: " << e.what() << std::endl;
                std::cerr << "File content: " << content << std::endl;
            }
        }
    }

    if (needs_init) {
        std::ofstream json_file("original_sender.json");
        if (json_file.is_open()) {
            json_file << "[]";
            json_file.close();
            std::cout << "Initialized original_sender.json as empty array" << std::endl;
        } else {
            std::cerr << "Failed to initialize original_sender.json" << std::endl;
            return 1;
        }
    }

    UDPSimulator udpSimulator("127.0.0.1", 5000);
    ICSimulator icSimulator;
    FlexRaySimulator flexRaySimulator;

    const float max_speed = 78.0f;
    const float max_rpm = 8000.0f;
    const float speed_step = 5.0f;
    const float rpm_step = 100.0f;

    simulateFloatData(
        [&udpSimulator, &icSimulator, &flexRaySimulator](float speed, float rpm) {
            udpSimulator.sendUDPData(speed, rpm);
            icSimulator.sendCombinedData(speed, rpm);
            flexRaySimulator.sendCombinedData(speed, rpm);
        },
        0.0f, max_speed, 0.0f, max_rpm, speed_step, rpm_step, std::chrono::milliseconds(100)
    );

    return 0;
}