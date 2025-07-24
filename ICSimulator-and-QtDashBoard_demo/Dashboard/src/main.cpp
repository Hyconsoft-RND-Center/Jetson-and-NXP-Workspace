#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QFont>
#include <QFontDatabase>
#include <QThread>
#include <QDebug>
#include <QHostAddress>
#include <QDateTime>
#include <QTcpSocket>
#include <QFile>
#include <QFileInfo>
#include <arpa/inet.h>
#include "UdpReceiver.h"
#include "CanReceiver.h"
#include "TcpSignalReceiver.h"

// Define ENABLE_FLEXRAY and ENABLE_LIN (0 = disabled, 1 = enabled)
#define ENABLE_FLEXRAY 1
#define ENABLE_LIN 1
#if ENABLE_FLEXRAY
#include "FlexrayReceiver.h"
#endif
#if ENABLE_LIN
#include "LinReceiver.h"
#endif

// Logging configuration macros
#define ENABLE_DEBUG_LOGGING    1
#define ENABLE_INFO_LOGGING     1
#define ENABLE_WARNING_LOGGING  1
#define ENABLE_CRITICAL_LOGGING 1

#if !ENABLE_DEBUG_LOGGING
#define QT_NO_DEBUG_OUTPUT
#endif
#if !ENABLE_INFO_LOGGING
#define QT_NO_INFO_OUTPUT
#endif
#if !ENABLE_WARNING_LOGGING
#define QT_NO_WARNING_OUTPUT
#endif
#if !ENABLE_CRITICAL_LOGGING
#define qCritical() QT_NO_QDEBUG_MACRO()
#endif

void sendJsonFileOverTcp(const QString& filename, const QString& host, quint16 port, int attempt = 1, const int maxRetries = 3, const int retryDelayMs = 500) {
    QTcpSocket socket;
    socket.connectToHost(host, port, QIODevice::WriteOnly);
    if (!socket.waitForConnected(2000)) {
        qWarning() << QString("Attempt %1: Failed to connect to %2:%3 for %4").arg(attempt).arg(host).arg(port).arg(filename);
        if (attempt < maxRetries) {
            QThread::msleep(retryDelayMs);
            sendJsonFileOverTcp(filename, host, port, attempt + 1, maxRetries, retryDelayMs);
        } else {
            qWarning() << QString("Failed to connect to %1:%2 for %3 after %4 attempts").arg(host).arg(port).arg(filename).arg(maxRetries);
        }
        return;
    }

    // Open and read the JSON file
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << QString("Failed to open %1: %2").arg(filename).arg(file.errorString());
        socket.close();
        return;
    }
    QByteArray fileData = file.readAll();
    file.close();

    if (fileData.isEmpty()) {
        qWarning() << QString("JSON file is empty: %1").arg(filename);
        socket.close();
        return;
    }

    QString baseFilename = QFileInfo(filename).fileName();
    QByteArray filenameBytes = baseFilename.toUtf8();
    uint32_t filenameLength = htonl(filenameBytes.size());
    QByteArray header;
    header.append(reinterpret_cast<const char*>(&filenameLength), sizeof(filenameLength));
    header.append(filenameBytes);

    // Combine header and file data
    QByteArray dataToSend = header + fileData;
    qint64 bytesWritten = socket.write(dataToSend);
    socket.waitForBytesWritten(2000);
    if (bytesWritten != dataToSend.size()) {
        qWarning() << QString("Attempt %1: Failed to send %2: %3 bytes written").arg(attempt).arg(filename).arg(bytesWritten);
        if (attempt < maxRetries) {
            QThread::msleep(retryDelayMs);
            socket.close();
            sendJsonFileOverTcp(filename, host, port, attempt + 1, maxRetries, retryDelayMs);
        } else {
            qWarning() << QString("Failed to send %1 after %2 attempts").arg(filename).arg(maxRetries);
        }
    } else {
        qInfo() << QString("Successfully sent %1 to %2:%3 (%4 bytes)").arg(filename).arg(host).arg(port).arg(bytesWritten);
    }
    socket.close();
}

void clearJsonFiles(const QStringList& jsonFiles) {
    for (const QString& file : jsonFiles) {
        QFile qfile(file);
        if (qfile.exists()) {
            if (qfile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                qfile.write("[]");
                qfile.close();
                qInfo() << QString("Cleared JSON file: %1").arg(file);
            } else {
                qWarning() << QString("Failed to clear JSON file: %1").arg(file);
            }
        } else {
            if (qfile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                qfile.write("[]");
                qfile.close();
                qInfo() << QString("Created and initialized JSON file: %1").arg(file);
            } else {
                qWarning() << QString("Failed to create JSON file: %1").arg(file);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    // Initializes Qt application and sets up CAN, UDP, and FlexRay receivers
    // IPs: Two IPs provided via command-line:
    //   - argv[1]: Qt’s own IP (e.g., 192.168.0.48 or 127.0.0.1 for local)
    //   - argv[2]: Autoware’s IP (e.g., 192.168.0.6 or 127.0.0.1 for local)
    // Port: argv[3] (e.g., 5000 for UDP receiver and JSON file sending, 5001 for TCP signal receiver)
    // For testing with 192.168.x.x IPs:
    // - Use Qt’s IP (e.g., 192.168.0.48) for binding UDP/TCP receivers
    // - Use Autoware’s IP (e.g., 192.168.0.6) for sending JSON files
    // - Command-line example: ./dashboard 192.168.0.48 192.168.0.6 5000
    QGuiApplication app(argc, argv);
    qSetMessagePattern("[%{type}] %{time yyyy-MM-dd HH:mm:ss.zzz} %{message}");

    // Validate command-line arguments
    if (argc != 4) {
        qFatal("Usage: %s <own_ip> <autoware_ip> <port>", argv[0]);
        return -1;
    }
    QString ipAddress = argv[1]; // Qt’s own IP (e.g., 192.168.0.48 or 127.0.0.1)
    QString autowareIp = argv[2]; // Autoware’s IP (e.g., 192.168.0.6 or 127.0.0.1)
    bool ok;
    quint16 port = QString(argv[3]).toUShort(&ok);
    if (!ok) {
        qFatal("Invalid port: %s", argv[3]);
        return -1;
    }

    // Validate IP addresses
    QHostAddress address(ipAddress);
    QHostAddress autowareAddress(autowareIp);
    if (address.isNull() || autowareAddress.isNull()) {
        qFatal("Invalid IP: %s or %s", ipAddress.toStdString().c_str(), autowareIp.toStdString().c_str());
        return -1;
    }

    // Initialize threads for receivers
    QThread *canThread = new QThread;
    QThread *udpThread = new QThread;
    QThread *tcpThread = new QThread;

    // Set up CAN receiver (no IP/port, uses vcan0 interface)
    CanReceiver *canReceiver = new CanReceiver("can2");

    // Set up UDP receiver
    // IP: Binds to Qt’s own IP (ipAddress, e.g., 192.168.0.48 or 127.0.0.1)
    // Port: Uses port from argv[3] (e.g., 5000)
    // Single IP/port for receiving UDP data from vehicle signals
    // For testing with 192.168.x.x IPs:
    // - Ensure port 5000 is open on Qt’s firewall
    UdpReceiver *udpReceiver = new UdpReceiver(ipAddress, port);

    // Set up TCP signal receiver for SEND_JSON signal
    // IP: Binds to Qt’s own IP (ipAddress, e.g., 192.168.0.48 or 127.0.0.1)
    // Port: 5001 (hardcoded) to avoid conflict with UDP port (5000)
    // Single IP/port for receiving SEND_JSON signal from Autoware
    // For testing with 192.168.x.x IPs:
    // - Ensure port 5001 is open on Qt’s firewall
    // - Autoware sends to 192.168.0.48:5001
    TcpSignalReceiver *tcpReceiver = new TcpSignalReceiver(ipAddress, 5001);

#if ENABLE_FLEXRAY
    // Set up FlexRay receiver
    // IP: Binds to Qt’s own IP (ipAddress, e.g., 192.168.0.48 or 127.0.0.1)
    // Port: 5002 (hardcoded) to avoid conflicts with UDP (5000) and TCP (5001)
    // Single IP/port for receiving FlexRay data
    // For testing with 192.168.x.x IPs:
    // - Ensure port 5002 is open on Qt’s firewall
    QThread *flexrayThread = new QThread;
    FlexRayReceiver *flexrayReceiver = new FlexRayReceiver(ipAddress, 5002);
    flexrayReceiver->moveToThread(flexrayThread);
    flexrayThread->start();
#endif
#if ENABLE_LIN
    // Set up LIN receiver (no IP/port, not implemented)
    QThread *linThread = new QThread;
    LinReceiver *linReceiver = new LinReceiver;
    linReceiver->moveToThread(linThread);
    linThread->start();
#endif

    // Move receivers to their respective threads
    canReceiver->moveToThread(canThread);
    udpReceiver->moveToThread(udpThread);
    tcpReceiver->moveToThread(tcpThread);
    canThread->start();
    udpThread->start();
    tcpThread->start();

    QStringList jsonFiles = {
        "can_protocol_receiver.json",
        "udp_protocol_receiver.json"
#if ENABLE_FLEXRAY
        ,"flexray_protocol_receiver.json"
#endif
#if ENABLE_LIN
        ,"lin_protocol_receiver.json"
#endif
    };
    clearJsonFiles(jsonFiles);

    QFontDatabase::addApplicationFont(":/resources/fonts/DejaVuSans.ttf");
    app.setFont(QFont("DejaVu Sans"));
    QQmlApplicationEngine engine(QUrl("qrc:/resources/qml/dashboard.qml"));
    if (engine.rootObjects().isEmpty()) {
        qFatal("Failed to load QML dashboard");
        return -1;
    }

    // Connect QML ValueSource objects to receivers
    QObject *rootObject = engine.rootObjects().first();
    QObject *receiver1 = nullptr;
    QObject *receiver2 = nullptr;
#if ENABLE_LIN
    QObject *receiver3 = nullptr;
#endif
#if ENABLE_FLEXRAY
    QObject *receiver4 = nullptr;
#endif
    QList<QObject *> children = rootObject->findChildren<QObject *>();
    for (QObject *child : children) {
        QString className = child->metaObject()->className();
        if (className.contains("ValueSource")) {
            QString objectName = child->objectName();
            if (objectName == "receiver1") receiver1 = child;
            else if (objectName == "receiver2") receiver2 = child;
#if ENABLE_LIN
            else if (objectName == "receiver3") receiver3 = child;
#endif
#if ENABLE_FLEXRAY
            else if (objectName == "receiver4") receiver4 = child;
#endif
        }
    }
    if (receiver1 && receiver2
#if ENABLE_LIN
        && receiver3
#endif
#if ENABLE_FLEXRAY
        && receiver4
#endif
    ) {
        // Connect UDP receiver signals to receiver1 (QML)
        QObject::connect(udpReceiver, &UdpReceiver::speedDataReceived, receiver1,
            [&receiver1](float speed) { receiver1->setProperty("kph", speed); });
        QObject::connect(udpReceiver, &UdpReceiver::rpmDataReceived, receiver1,
            [&receiver1](float rpm) { receiver1->setProperty("rpm", rpm); });
        QObject::connect(udpReceiver, &UdpReceiver::fuelDataReceived, receiver1,
            [&receiver1](float fuel) { receiver1->setProperty("fuel", fuel); });
        QObject::connect(udpReceiver, &UdpReceiver::tempDataReceived, receiver1,
            [&receiver1](float temp) { receiver1->setProperty("temperature", temp); });

        // Connect CAN receiver signals to receiver2 (QML)
        QObject::connect(canReceiver, &CanReceiver::speedDataReceived, receiver2,
            [&receiver2](float speed) { receiver2->setProperty("kph", speed); });
        QObject::connect(canReceiver, &CanReceiver::rpmDataReceived, receiver2,
            [&receiver2](float rpm) { receiver2->setProperty("rpm", rpm); });
        QObject::connect(canReceiver, &CanReceiver::fuelDataReceived, receiver2,
            [&receiver2](float fuel) { receiver2->setProperty("fuel", fuel); });
        QObject::connect(canReceiver, &CanReceiver::tempDataReceived, receiver2,
            [&receiver2](float temp) { receiver2->setProperty("temperature", temp); });

#if ENABLE_LIN
        // Connect LIN receiver signals to receiver3 (QML)
        QObject::connect(linReceiver, &LinReceiver::speedDataReceived, receiver3,
            [&receiver3](float speed) { receiver3->setProperty("kph", speed); });
        QObject::connect(linReceiver, &LinReceiver::rpmDataReceived, receiver3,
            [&receiver3](float rpm) { receiver3->setProperty("rpm", rpm); });
        QObject::connect(linReceiver, &LinReceiver::fuelDataReceived, receiver3,
            [&receiver3](float fuel) { receiver3->setProperty("fuel", fuel); });
        QObject::connect(linReceiver, &LinReceiver::tempDataReceived, receiver3,
            [&receiver3](float temp) { receiver3->setProperty("temperature", temp); });
#endif
#if ENABLE_FLEXRAY
        // Connect FlexRay receiver signals to receiver4 (QML)
        QObject::connect(flexrayReceiver, &FlexRayReceiver::speedDataReceived, receiver4,
            [&receiver4](float speed) { receiver4->setProperty("kph", speed); });
        QObject::connect(flexrayReceiver, &FlexRayReceiver::rpmDataReceived, receiver4,
            [&receiver4](float rpm) { receiver4->setProperty("rpm", rpm); });
        QObject::connect(flexrayReceiver, &FlexRayReceiver::fuelDataReceived, receiver4,
            [&receiver4](float fuel) { receiver4->setProperty("fuel", fuel); });
        QObject::connect(flexrayReceiver, &FlexRayReceiver::tempDataReceived, receiver4,
            [&receiver4](float temp) { receiver4->setProperty("temperature", temp); });
#endif
    } else {
        qWarning() << "Not all required ValueSource objects found!";
        if (!receiver1) qWarning() << "Receiver1 not found!";
        if (!receiver2) qWarning() << "Receiver2 not found!";
#if ENABLE_LIN
        if (!receiver3) qWarning() << "Receiver3 not found!";
#endif
#if ENABLE_FLEXRAY
        if (!receiver4) qWarning() << "Receiver4 not found!";
#endif
    }

    auto resetReceivers = [&]() {
        canThread->quit();
        canThread->wait();
        delete canReceiver;
        canThread->deleteLater();
        canThread = new QThread;
        canReceiver = new CanReceiver("can2");
        canReceiver->moveToThread(canThread);
        canThread->start();

        udpThread->quit();
        udpThread->wait();
        delete udpReceiver;
        udpThread->deleteLater();
        udpThread = new QThread;
        udpReceiver = new UdpReceiver(ipAddress, port);
        udpReceiver->moveToThread(udpThread);
        udpThread->start();

#if ENABLE_FLEXRAY
        flexrayThread->quit();
        flexrayThread->wait();
        delete flexrayReceiver;
        flexrayThread->deleteLater();
        flexrayThread = new QThread;
        flexrayReceiver = new FlexRayReceiver(ipAddress, 5002);
        flexrayReceiver->moveToThread(flexrayThread);
        flexrayThread->start();
#endif

        // Reconnect signals for new receiver instances
        if (receiver1) {
            QObject::connect(udpReceiver, &UdpReceiver::speedDataReceived, receiver1,
                [&receiver1](float speed) { receiver1->setProperty("kph", speed); });
            QObject::connect(udpReceiver, &UdpReceiver::rpmDataReceived, receiver1,
                [&receiver1](float rpm) { receiver1->setProperty("rpm", rpm); });
            QObject::connect(udpReceiver, &UdpReceiver::fuelDataReceived, receiver1,
                [&receiver1](float fuel) { receiver1->setProperty("fuel", fuel); });
            QObject::connect(udpReceiver, &UdpReceiver::tempDataReceived, receiver1,
                [&receiver1](float temp) { receiver1->setProperty("temperature", temp); });
        }
        if (receiver2) {
            QObject::connect(canReceiver, &CanReceiver::speedDataReceived, receiver2,
                [&receiver2](float speed) { receiver2->setProperty("kph", speed); });
            QObject::connect(canReceiver, &CanReceiver::rpmDataReceived, receiver2,
                [&receiver2](float rpm) { receiver2->setProperty("rpm", rpm); });
            QObject::connect(canReceiver, &CanReceiver::fuelDataReceived, receiver2,
                [&receiver2](float fuel) { receiver2->setProperty("fuel", fuel); });
            QObject::connect(canReceiver, &CanReceiver::tempDataReceived, receiver2,
                [&receiver2](float temp) { receiver2->setProperty("temperature", temp); });
        }
#if ENABLE_FLEXRAY
        if (receiver4) {
            QObject::connect(flexrayReceiver, &FlexRayReceiver::speedDataReceived, receiver4,
                [&receiver4](float speed) { receiver4->setProperty("kph", speed); });
            QObject::connect(flexrayReceiver, &FlexRayReceiver::rpmDataReceived, receiver4,
                [&receiver4](float rpm) { receiver4->setProperty("rpm", rpm); });
            QObject::connect(flexrayReceiver, &FlexRayReceiver::fuelDataReceived, receiver4,
                [&receiver4](float fuel) { receiver4->setProperty("fuel", fuel); });
            QObject::connect(flexrayReceiver, &FlexRayReceiver::tempDataReceived, receiver4,
                [&receiver4](float temp) { receiver4->setProperty("temperature", temp); });
        }
#endif
        qInfo() << "Receiver threads reset, ready for new data";
    };

    QObject::connect(tcpReceiver, &TcpSignalReceiver::sendJsonFilesRequested, [&]() {
        qInfo() << "Received SEND_JSON, sending JSON files to" << autowareIp << ":" << port;
        QThread::msleep(100);
        for (const QString& file : jsonFiles) {
            if (QFile::exists(file)) {
                sendJsonFileOverTcp(file, autowareIp, port);
            } else {
                qWarning() << QString("JSON file does not exist: %1").arg(file);
            }
        }
        qInfo() << "JSON files sent";
    });

    QObject::connect(tcpReceiver, &TcpSignalReceiver::receivedJsonSignal, [&]() {
        qInfo() << "Received RECEIVED_JSON, resetting application state";
        clearJsonFiles(jsonFiles);
        resetReceivers();
    });

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        canThread->quit();
        udpThread->quit();
        tcpThread->quit();
#if ENABLE_FLEXRAY
        flexrayThread->quit();
        flexrayThread->wait();
        delete flexrayReceiver;
        delete flexrayThread;
#endif
#if ENABLE_LIN
        linThread->quit();
        linThread->wait();
        delete linReceiver;
        delete linThread;
#endif
        canThread->wait();
        udpThread->wait();
        tcpThread->wait();
        delete canReceiver;
        delete udpReceiver;
        delete tcpReceiver;
        delete canThread;
        delete udpThread;
        delete tcpThread;
    });

    return app.exec();
}
