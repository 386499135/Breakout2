#include "NetworkManager.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

NetworkManager::NetworkManager() : socket_fd(-1), connected(false), running(false) {}

NetworkManager::~NetworkManager() {
    Disconnect();
}

bool NetworkManager::StartHost(int port) {
    // 创建 UDP socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(socket_fd);
        return false;
    }
    
    role = "HOST";
    connected = true;
    running = true;
    
    // 启动接收线程
    recvThread = std::thread([this]() {
        char buffer[1024];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10000;
        setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        while (running) {
            int len = recvfrom(socket_fd, buffer, sizeof(buffer) - 1, 0,
                              (struct sockaddr*)&from_addr, &from_len);
            
            if (len > 0) {
                buffer[len] = '\0';
                std::string msg(buffer);
                
                if (msg.find("PADDLE") == 0) {
                    float x = std::stof(msg.substr(7));
                    remotePaddleX = x;
                    if (onRemotePaddleUpdate) onRemotePaddleUpdate(x);
                }
                
                if (remoteIP.empty()) {
                    remoteIP = inet_ntoa(from_addr.sin_addr);
                    remotePort = ntohs(from_addr.sin_port);
                    std::cout << "Client connected from " << remoteIP << ":" << remotePort << std::endl;
                }
            }
        }
    });
    
    std::cout << "Host started on port " << port << std::endl;
    return true;
}

bool NetworkManager::ConnectToHost(const std::string& ip, int port) {
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    remoteIP = ip;
    remotePort = port;
    role = "CLIENT";
    connected = true;
    running = true;
    
    // 启动接收线程
    recvThread = std::thread([this]() {
        char buffer[1024];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10000;
        setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        while (running) {
            int len = recvfrom(socket_fd, buffer, sizeof(buffer) - 1, 0,
                              (struct sockaddr*)&from_addr, &from_len);
            
            if (len > 0) {
                buffer[len] = '\0';
                std::string msg(buffer);
                
                if (msg.find("PADDLE") == 0) {
                    float x = std::stof(msg.substr(7));
                    remotePaddleX = x;
                    if (onRemotePaddleUpdate) onRemotePaddleUpdate(x);
                }
            }
        }
    });
    
    std::cout << "Connecting to " << ip << ":" << port << std::endl;
    return true;
}

void NetworkManager::Disconnect() {
    running = false;
    connected = false;
    
    if (recvThread.joinable()) {
        recvThread.join();
    }
    
    if (socket_fd >= 0) {
        close(socket_fd);
        socket_fd = -1;
    }
    
    role = "";
    remoteIP = "";
    remotePort = 0;
}

void NetworkManager::Update() {
    // 不需要额外更新
}

void NetworkManager::SendPaddlePosition(float x) {
    if (!connected || socket_fd < 0) return;
    
    std::string msg = "PADDLE" + std::to_string(x);
    
    if (role == "HOST" && !remoteIP.empty()) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(remotePort);
        inet_pton(AF_INET, remoteIP.c_str(), &addr.sin_addr);
        sendto(socket_fd, msg.c_str(), msg.length(), 0,
               (struct sockaddr*)&addr, sizeof(addr));
    } 
    else if (role == "CLIENT" && !remoteIP.empty()) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(remotePort);
        inet_pton(AF_INET, remoteIP.c_str(), &addr.sin_addr);
        sendto(socket_fd, msg.c_str(), msg.length(), 0,
               (struct sockaddr*)&addr, sizeof(addr));
    }
}
