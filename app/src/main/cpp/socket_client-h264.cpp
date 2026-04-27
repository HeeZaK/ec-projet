#include "socket_client-h264.h"
#include <vector>
#include <cstring>
#include <cerrno>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>

SocketClientH264::SocketClientH264(const std::string& ip, int port)
    : m_server_ip(ip), m_port(port), m_socket_fd(-1), m_is_connected(false) {
}

SocketClientH264::~SocketClientH264() {
    Disconnect();
}

bool SocketClientH264::ConnectToServer() {
    LOGI("TCP: Tentative de connexion vers %s:%d", m_server_ip.c_str(), m_port);
    m_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket_fd < 0) return false;

    // Mode non-bloquant pour le timeout
    int flags = fcntl(m_socket_fd, F_GETFL, 0);
    fcntl(m_socket_fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(static_cast<uint16_t>(m_port));
    inet_pton(AF_INET, m_server_ip.c_str(), &server_addr.sin_addr);

    int res = connect(m_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (res < 0 && errno == EINPROGRESS) {
        fd_set wait_set;
        FD_ZERO(&wait_set);
        FD_SET(m_socket_fd, &wait_set);
        struct timeval tv = {5, 0};
        res = select(m_socket_fd + 1, NULL, &wait_set, NULL, &tv);
    }

    if (res > 0) {
        fcntl(m_socket_fd, F_SETFL, flags);
        m_is_connected = true;
        LOGI("TCP: CONNECTE !");
        return true;
    }
    close(m_socket_fd);
    m_socket_fd = -1;
    return false;
}

void SocketClientH264::Disconnect() {
    if (m_socket_fd != -1) {
        close(m_socket_fd);
        m_socket_fd = -1;
    }
    m_is_connected = false;
}

bool SocketClientH264::SendImageH264(const void* data, size_t size) {
    return SendData(data, size);
}

bool SocketClientH264::SendData(const void* data, size_t size) {
    if (m_socket_fd == -1 || !m_is_connected) return false;

    size_t total_sent = 0;
    const uint8_t* ptr = static_cast<const uint8_t*>(data);

    while (total_sent < size) {
        ssize_t sent = send(m_socket_fd, ptr + total_sent, size - total_sent, 0);
        if (sent <= 0) {
            m_is_connected = false;
            return false;
        }
        total_sent += sent;
    }
    return true;
}
