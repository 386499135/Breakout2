#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <string>
#include <functional>
#include <atomic>
#include <thread>

class NetworkManager {
private:
    int socket_fd;
    std::atomic<bool> connected;
    std::atomic<bool> running;
    std::string role;
    std::string remoteIP;
    int remotePort;
    std::thread recvThread;
    std::atomic<float> remotePaddleX;
    
public:
    NetworkManager();
    ~NetworkManager();
    
    bool StartHost(int port = 12345);
    bool ConnectToHost(const std::string& ip, int port = 12345);
    void Disconnect();
    void Update();
    
    void SendPaddlePosition(float x);
    float GetRemotePaddleX() const { return remotePaddleX.load(); }
    bool IsConnected() const { return connected.load(); }
    std::string GetRole() const { return role; }
    
    std::function<void(float)> onRemotePaddleUpdate;
};

#endif
