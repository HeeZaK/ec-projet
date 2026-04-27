#ifndef ECPROJECT_SOCKET_CLIENT_H264_H
#define ECPROJECT_SOCKET_CLIENT_H264_H

#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Util.h"

class SocketClientH264 {
public:
    SocketClientH264(const std::string& ip, int port);
    ~SocketClientH264();

    bool ConnectToServer();
    void Disconnect();

    // Méthode appelée par l'encodeur du prof
    bool SendImageH264(const void* data, size_t size);

    bool m_is_connected;

private:
    std::string m_server_ip;
    int m_port;
    int m_socket_fd;

    bool SendData(const void* data, size_t size);
};

#endif //ECPROJECT_SOCKET_CLIENT_H264_H
