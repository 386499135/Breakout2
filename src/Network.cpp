#include "Network.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>

NetworkManager::NetworkManager() {}

NetworkManager::~NetworkManager() {
    Disconnect();
}

bool NetworkManager::StartHost(int port) {
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
    
    role = Role::HOST;
    connected = true;
    running = true;
    
    networkThread = std::thread(&NetworkManager::NetworkThread, this);
    
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
    
    role = Role::CLIENT;
    connected = true;
    running = true;
    
    json msg = {{"type", "connect"}};
    SendJson(msg);
    
    networkThread = std::thread(&NetworkManager::NetworkThread, this);
    
    std::cout << "Connecting to " << ip << ":" << port << std::endl;
    return true;
}

void NetworkManager::Disconnect() {
    running = false;
    connected = false;
    
    if (networkThread.joinable()) {
        networkThread.join();
    }
    
    if (socket_fd >= 0) {
        close(socket_fd);
        socket_fd = -1;
    }
    
    role = Role::NONE;
}

void NetworkManager::Update() {}

void NetworkManager::NetworkThread() {
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
            try {
                json msg = json::parse(buffer);
                ProcessMessage(msg);
                
                if (role == Role::HOST && remoteIP.empty()) {
                    remoteIP = inet_ntoa(from_addr.sin_addr);
                    remotePort = ntohs(from_addr.sin_port);
                    if (onPlayerConnected) onPlayerConnected();
                }
            } catch (...) {}
        }
    }
}

void NetworkManager::ProcessMessage(const json& msg) {
    std::string type = msg.value("type", "");
    
    if (type == "paddle") {
        float x = msg.value("x", 400.0f);
        remotePaddleX = x;
        if (onPaddleUpdate) onPaddleUpdate(x);
    }
    else if (type == "ball") {
        float x = msg.value("x", 0.0f);
        float y = msg.value("y", 0.0f);
        float vx = msg.value("vx", 0.0f);
        float vy = msg.value("vy", 0.0f);
        if (onBallUpdate) onBallUpdate(x, y, vx, vy);
    }
    else if (type == "event") {
        std::string event = msg.value("event", "");
        json data = msg.value("data", json::object());
        if (onGameEvent) onGameEvent(event, data);
    }
    else if (type == "connect" && role == Role::HOST) {
        json reply = {{"type", "connected"}};
        SendJson(reply);
    }
}

void NetworkManager::SendJson(const json& data) {
    if (!connected) return;
    
    std::string str = data.dump();
    
    if (role == Role::HOST && !remoteIP.empty()) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(remotePort);
        inet_pton(AF_INET, remoteIP.c_str(), &addr.sin_addr);
        
        sendto(socket_fd, str.c_str(), str.length(), 0,
               (struct sockaddr*)&addr, sizeof(addr));
    }
    else if (role == Role::CLIENT) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(remotePort);
        inet_pton(AF_INET, remoteIP.c_str(), &addr.sin_addr);
        
        sendto(socket_fd, str.c_str(), str.length(), 0,
               (struct sockaddr*)&addr, sizeof(addr));
    }
}

void NetworkManager::SendPaddlePosition(float x) {
    json msg = {{"type", "paddle"}, {"x", x}};
    SendJson(msg);
}

void NetworkManager::SendBallState(float x, float y, float vx, float vy) {
    json msg = {{"type", "ball"}, {"x", x}, {"y", y}, {"vx", vx}, {"vy", vy}};
    SendJson(msg);
}

void NetworkManager::SendGameEvent(const std::string& event, const json& data) {
    json msg = {{"type", "event"}, {"event", event}, {"data", data}};
    SendJson(msg);
}
