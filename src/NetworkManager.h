#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "NetworkProtocol.h"
#include <enet/enet.h>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <chrono>

class NetworkManager {
public:
    enum class Role {
        NONE,
        HOST,
        CLIENT
    };
    
    struct RemotePlayer {
        ENetPeer* peer;
        uint8_t playerId;
        PaddleState paddleState;
        float lastUpdateTime;
        float latency;
        bool connected;
    };
    
    struct NetworkConfig {
        uint16_t port = 1234;
        uint32_t maxPlayers = 4;
        uint32_t maxChannels = 2;
        uint32_t incomingBandwidth = 0;  // unlimited
        uint32_t outgoingBandwidth = 0;  // unlimited
        float updateRate = 60.0f;        // updates per second
        float interpolationDelay = 0.1f;  // 100ms interpolation buffer
        bool simulatePacketLoss = false;
        float packetLossRate = 0.1f;     // 10% packet loss
    };
    
private:
    ENetHost* host;
    Role role;
    NetworkConfig config;
    
    std::vector<RemotePlayer> remotePlayers;
    std::queue<std::vector<uint8_t>> receiveQueue;
    std::queue<std::vector<uint8_t>> sendQueue;
    
    // 插值缓冲区
    struct SnapshotBuffer {
        std::vector<BallState> ballStates;
        float timestamp;
    };
    std::deque<SnapshotBuffer> snapshotBuffer;
    
    uint16_t sequenceNumber;
    float lastUpdateTime;
    float connectionTimer;
    
    // 回调函数
    std::function<void(uint8_t, const PaddleState&)> onPaddleUpdate;
    std::function<void(const std::vector<BallState>&)> onBallUpdate;
    std::function<void(uint8_t, uint8_t)> onBrickDestroyed;
    std::function<void(uint8_t, uint8_t, Vector2)> onPowerUpSpawn;
    std::function<void(uint8_t)> onPlayerConnected;
    std::function<void(uint8_t)> onPlayerDisconnected;
    std::function<void(uint8_t)> onHostMigrated;
    
public:
    NetworkManager();
    ~NetworkManager();
    
    bool Initialize();
    void Shutdown();
    
    // 网络角色设置
    bool StartHost();
    bool ConnectToHost(const char* address);
    void Disconnect();
    
    // 状态更新
    void Update(float deltaTime);
    void SendPaddleState(const PaddleState& state);
    void SendBallState(const std::vector<BallState>& balls);
    void SendBrickDestroyed(uint8_t brickIndex, Vector2 position);
    void SendPowerUpSpawn(uint8_t type, Vector2 position);
    
    // 插值处理
    void ProcessIncomingPackets();
    void InterpolateGameState(float deltaTime);
    void SimulatePacketLoss(bool enabled, float rate);
    
    // 主机迁移
    void CheckHostMigration();
    void MigrateToNewHost(uint8_t newHostId);
    
    // 回调设置
    void SetOnPaddleUpdate(std::function<void(uint8_t, const PaddleState&)> callback) {
        onPaddleUpdate = callback;
    }
    void SetOnBallUpdate(std::function<void(const std::vector<BallState>&)> callback) {
        onBallUpdate = callback;
    }
    void SetOnBrickDestroyed(std::function<void(uint8_t, uint8_t)> callback) {
        onBrickDestroyed = callback;
    }
    void SetOnPowerUpSpawn(std::function<void(uint8_t, uint8_t, Vector2)> callback) {
        onPowerUpSpawn = callback;
    }
    void SetOnPlayerConnected(std::function<void(uint8_t)> callback) {
        onPlayerConnected = callback;
    }
    void SetOnPlayerDisconnected(std::function<void(uint8_t)> callback) {
        onPlayerDisconnected = callback;
    }
    
    // 状态查询
    Role GetRole() const { return role; }
    uint8_t GetLocalPlayerId() const { return role == Role::HOST ? 0 : 1; }
    const std::vector<RemotePlayer>& GetRemotePlayers() const { return remotePlayers; }
    bool IsConnected() const { return host != nullptr && !remotePlayers.empty(); }
    float GetAverageLatency() const;
    
    // 调试功能
    void EnablePacketLossSimulation(bool enabled, float rate = 0.1f);
    void PrintNetworkStats() const;
};

#endif
