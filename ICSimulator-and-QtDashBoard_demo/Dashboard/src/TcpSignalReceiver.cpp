#include "TcpSignalReceiver.h"

// Constructor: Initializes the TCP server to listen on the specified IP and port
TcpSignalReceiver::TcpSignalReceiver(const QString &ip, quint16 port, QObject *parent)
    : QObject(parent), ip_(ip), port_(port) {
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &TcpSignalReceiver::handleNewConnection);

    QHostAddress address(ip);
    if (!server->listen(address, port)) {
        qFatal("Failed to start TCP server on %s:%d: %s",
               ip.toStdString().c_str(), port, server->errorString().toStdString().c_str());
    } else {
        qDebug() << "TCP server started on" << ip << ":" << port;
    }
}

// Destructor: Closes the TCP server
TcpSignalReceiver::~TcpSignalReceiver() {
    server->close();
}

// Handles new TCP connections
void TcpSignalReceiver::handleNewConnection() {
    while (server->hasPendingConnections()) {
        QTcpSocket *client = server->nextPendingConnection();
        connect(client, &QTcpSocket::readyRead, this, &TcpSignalReceiver::readClientData);
        connect(client, &QTcpSocket::disconnected, client, &QTcpSocket::deleteLater);
    }
}

// Reads data from connected clients and processes SEND_JSON signal
void TcpSignalReceiver::readClientData() {
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if (!client) return;

    QByteArray data = client->readAll();
    QString signal = QString::fromUtf8(data).trimmed();
    if (signal == "SEND_JSON") {
        qDebug() << "Received SEND_JSON from" << client->peerAddress().toString();
        emit sendJsonFilesRequested();
    } else if (signal == "RECEIVED_JSON") {
        qDebug() << "Received RECEIVED_JSON from" << client->peerAddress().toString();
        emit receivedJsonSignal();
    } else {
        qWarning() << "Received invalid signal from" << client->peerAddress().toString() << ":" << signal;
    }
    client->disconnectFromHost();
}
