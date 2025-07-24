#ifndef TCPSIGNALRECEIVER_H
#define TCPSIGNALRECEIVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QDebug>
#include <QtGlobal>

/**
 * TcpSignalReceiver manages a TCP server to listen for the SEND_JSON signal from Autoware.
 * Emits a signal to trigger JSON file transfer when SEND_JSON is received.
 */
class TcpSignalReceiver : public QObject {
    Q_OBJECT
public:
    // Constructor: Initializes the TCP server with specified IP and port
    explicit TcpSignalReceiver(const QString &ip, quint16 port, QObject *parent = nullptr);
    // Destructor: Cleans up server resources
    ~TcpSignalReceiver();

signals:
    // Signal emitted when SEND_JSON is received
    void sendJsonFilesRequested();
    void receivedJsonSignal();

private slots:
    // Handles new incoming connections
    void handleNewConnection();
    // Reads data from connected clients
    void readClientData();

private:
    QTcpServer *server; // TCP server instance
    QString ip_; // Server IP address
    quint16 port_; // Server port
};

#endif // TCPSIGNALRECEIVER_H
