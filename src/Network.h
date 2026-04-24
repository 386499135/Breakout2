#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class NetworkManager {
public:
    enum class Role { NONE, HOST, CLIENT };
    
    NetworkManager();
    ~NetworkManager();
    
    // 初始化和关闭
    bool StartHost(int port = 12345);
    bool ConnectToHost(const std::string& ip, int port = 12345);
    void Disconnect();
    void Update();
    
    // 发送数据
    void SendPaddlePosition(float x);
    void SendBallState(float x, float y, float vx, float vy);
    void SendGameEvent(const std::string& event, const json& data = {});
    
    // 回调设置
    void SetOnPaddleUpdate(std::function<void(float)> callback) { onPaddleUpdate = callback; }
    void SetOnBallUpdate(std::function<void(float, float, float, float)> callback) { onBallUpdate = callback; }
    void SetOnGameEvent(std::function<void(const std::string&, const json&)> callback) { onGameEvent = callback; }
    void SetOnPlayerConnected(std::function<void()> callback) { onPlayerConnected = callback; }
    
    // 状态查询
    Role GetRole() const { return role; }
    bool IsConnected() const { return connected; }
    float GetRemotePaddleX() const { return remotePaddleX; }
    
private:
    void NetworkThread();
    void ProcessMessage(const json& msg);
    void SendJson(const json& data);
    
    Role role = Role::NONE;
    std::atomic<bool> connected{false};
    std::atomic<bool> running{false};
    
    int socket_fd = -1;
    std::string remoteIP;
    int remotePort = 0;
    
    std::thread networkThread;
    std::mutex sendMutex;
    
    // 回调
    std::function<void(float)> onPaddleUpdate;
    std::function<void(float, float, float, float)> onBallUpdate;
    std::function<void(const std::string&, const json&)> onGameEvent;
    std::function<void()> onPlayerConnected;
    
    // 远程状态
    std::atomic<float> remotePaddleX{400};
};

#endif
