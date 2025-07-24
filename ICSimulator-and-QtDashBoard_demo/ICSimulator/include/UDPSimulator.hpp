#ifndef UDPSIMULATOR_HPP
#define UDPSIMULATOR_HPP

#include <string>
#include <fstream>
#include <netinet/in.h>  // Needed for struct sockaddr_in
#include <nlohmann/json.hpp>
#include <thread>  // Needed for std::this_thread::sleep_for    

class UDPSimulator
{
public:
    UDPSimulator(const std::string& ip = "127.0.0.1", int port = 5000);
    ~UDPSimulator();

    bool sendCombinedData(float speed, float rpm);
    bool sendUDPData(float speed, float rpm);

private:
    int m_socket;
    struct sockaddr_in m_serverAddr;
    std::ofstream protocol_sender_json;

    void closeSocket();
   
   // Logs signal data to JSON file
template<typename T>
void logSignalToJson(const std::string& signalName, const T& value);
};

#endif // UDPSIMULATOR_HPP
