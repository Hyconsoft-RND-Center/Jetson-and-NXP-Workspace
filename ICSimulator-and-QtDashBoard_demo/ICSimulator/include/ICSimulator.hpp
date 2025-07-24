#ifndef ICSIMULATOR_H
#define ICSIMULATOR_H

#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>

class ICSimulator
{
public:
    ICSimulator();
    ~ICSimulator();

    bool sendCombinedData(float speed, float rpm);
    // bool sendFuelData(float fuel);
    // bool sendTempData(float temp);

private:
    bool sendCANData(uint32_t canId, const void* data, size_t dataSize);
    template<typename T>
    void logSignalToJson(const std::string& signalName, const T& value);
    void closeSocket();

    int m_socket;
    std::ofstream protocol_sender_json;
};

#endif // ICSIMULATOR_H