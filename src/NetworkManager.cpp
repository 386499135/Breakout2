#include "NetworkManager.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <chrono>

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<float> lossDist(0.0f, 1.0f);

NetworkManager::NetworkManager() 
    : host(nullptr)
    , role(Role::NONE)
    , sequenceNumber(0)
    , lastUpdateTime(0.0f)
    , connectionTimer(0.0f) {
}

NetworkManager::~NetworkManager() {
    Shutdown();
}

bool NetworkManager::Initialize() {
    if (enet_initialize() != 0) {
        std::cerr << "Failed to initialize ENet!" << std::endl;
        return false;
    }
    return true;
}

void NetworkManager::Shutdown() {
    if (host) {
        enet_host_destroy(host);
        host = nullptr;
    }
    remotePlayers.clear();
    enet_deinitialize();
}

bool NetworkManager::StartHost() {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = config.port;
    
    host = enet_host_create(&address, config.maxPlayers, config.maxChannels, 
                           config.incomingBandwidth, config.outgoingBandwidth);
    
    if (!host) {
        std::cerr << "Failed to create ENet host!" << std::endl;
        return false;
    }
    
    role = Role::HOST;
    std::cout << "Host started on port " << config.port << std::endl;
    
    // 添加本地玩家（主机玩家0）
    RemotePlayer localPlayer;
    localPlayer.peer = nullptr;
    localPlayer.playerId = 0;
    localPlayer.connected = true;
    localPlayer.lastUpdateTime = 0.0f;
    remotePlayers.push_back(localPlayer);
    
    return true;
}

bool NetworkManager::ConnectToHost(const char* addressStr) {
    host = enet_host_create(nullptr, 1, config.maxChannels, 
                           config.incomingBandwidth, config.outgoingBandwidth);
    
    if (!host) {
        std::cerr << "Failed to create ENet client host!" << std::endl;
        return false;
    }
    
    ENetAddress address;
    enet_address_set_host(&address, addressStr);
    address.port = config.port;
    
    ENetPeer* peer = enet_host_connect(host, &address, config.maxChannels, 0);
    if (!peer) {
        std::cerr << "Failed to connect to host!" << std::endl;
        return false;
    }
    
    role = Role::CLIENT;
    std::cout << "Connecting to host at " << addressStr << ":" << config.port << std::endl;
    
    return true;
}

void NetworkManager::Update(float deltaTime) {
    if (!host) return;
    
    connectionTimer += deltaTime;
    lastUpdateTime += deltaTime;
    
    ENetEvent event;
    while (enet_host_service(host, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                HandleConnect(event);
                break;
                
            case ENET_EVENT_TYPE_RECEIVE:
                HandleReceive(event);
                break;
                
            case ENET_EVENT_TYPE_DISCONNECT:
                HandleDisconnect(event);
                break;
                
            default:
                break;
        }
    }
    
    // 处理接收到的数据包
    ProcessIncomingPackets();
    
    // 插值处理游戏状态
    if (role == Role::CLIENT) {
        InterpolateGameState(deltaTime);
    }
    
    // 主机检查迁移
    if (role == Role::HOST) {
        CheckHostMigration();
    }
    
    // 定期发送状态更新
    float updateInterval = 1.0f / config.updateRate;
    if (lastUpdateTime >= updateInterval) {
        if (role == Role::HOST) {
            SendGameState();
        } else if (role == Role::CLIENT) {
            SendPaddleUpdate();
        }
        lastUpdateTime = 0.0f;
    }
}

void NetworkManager::HandleConnect(ENetEvent& event) {
    if (role == Role::HOST) {
        // 检查是否有空位
        if (remotePlayers.size() < config.maxPlayers) {
            RemotePlayer player;
            player.peer = event.peer;
            player.playerId = remotePlayers.size();
            player.connected = true;
            player.lastUpdateTime = connectionTimer;
            
            remotePlayers.push_back(player);
            
            // 发送连接接受消息
            NetworkHeader header;
            header.messageType = (uint8_t)MessageType::CONNECTION_ACCEPTED;
            header.playerId = player.playerId;
            header.sequenceNumber = sequenceNumber++;
            header.timestamp = connectionTimer;
            
            ENetPacket* packet = enet_packet_create(&header, sizeof(header), 
                                                   ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(event.peer, 0, packet);
            
            std::cout << "Player " << (int)player.playerId << " connected" << std::endl;
            
            if (onPlayerConnected) {
                onPlayerConnected(player.playerId);
            }
        } else {
            // 服务器已满
            NetworkHeader header;
            header.messageType = (uint8_t)MessageType::SERVER_FULL;
            header.playerId = 0;
            header.sequenceNumber = sequenceNumber++;
            header.timestamp = connectionTimer;
            
            ENetPacket* packet = enet_packet_create(&header, sizeof(header), 
                                                   ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(event.peer, 0, packet);
            enet_peer_disconnect(event.peer, 0);
        }
    } else if (role == Role::CLIENT) {
        std::cout << "Connected to host" << std::endl;
    }
}

void NetworkManager::HandleReceive(ENetEvent& event) {
    // 模拟丢包
    if (config.simulatePacketLoss && lossDist(gen) < config.packetLossRate) {
        enet_packet_destroy(event.packet);
        return;
    }
    
    // 将数据包加入接收队列
    std::vector<uint8_t> data(event.packet->dataLength);
    memcpy(data.data(), event.packet->data, event.packet->dataLength);
    receiveQueue.push(std::move(data));
    
    enet_packet_destroy(event.packet);
}

void NetworkManager::HandleDisconnect(ENetEvent& event) {
    if (role == Role::HOST) {
        auto it = std::find_if(remotePlayers.begin(), remotePlayers.end(),
            [&event](const RemotePlayer& p) { return p.peer == event.peer; });
        
        if (it != remotePlayers.end()) {
            uint8_t playerId = it->playerId;
            remotePlayers.erase(it);
            
            std::cout << "Player " << (int)playerId << " disconnected" << std::endl;
            
            if (onPlayerDisconnected) {
                onPlayerDisconnected(playerId);
            }
        }
    } else if (role == Role::CLIENT) {
        std::cout << "Disconnected from host" << std::endl;
        role = Role::NONE;
    }
}

void NetworkManager::ProcessIncomingPackets() {
    while (!receiveQueue.empty()) {
        auto& data = receiveQueue.front();
        NetworkHeader* header = reinterpret_cast<NetworkHeader*>(data.data());
        
        switch (static_cast<MessageType>(header->messageType)) {
            case MessageType::GAME_STATE_UPDATE:
                ProcessGameStatePacket(data);
                break;
                
            case MessageType::PADDLE_POSITION:
                ProcessPaddlePacket(data);
                break;
                
            case MessageType::BALL_LAUNCH:
            case MessageType::BRICK_DESTROYED:
                ProcessBrickDestroyedPacket(data);
                break;
                
            case MessageType::POWERUP_SPAWN:
                ProcessPowerUpPacket(data);
                break;
                
            case MessageType::HOST_MIGRATION:
                ProcessHostMigrationPacket(data);
                break;
                
            case MessageType::PING:
                SendPong(header->playerId);
                break;
                
            default:
                break;
        }
        
        receiveQueue.pop();
    }
}

void NetworkManager::ProcessGameStatePacket(const std::vector<uint8_t>& data) {
    GameStatePacket* packet = (GameStatePacket*)data.data();
    
    // 创建快照并加入缓冲区
    SnapshotBuffer snapshot;
    snapshot.timestamp = packet->header.timestamp;
    
    for (int i = 0; i < packet->ballCount; i++) {
        snapshot.ballStates.push_back(packet->balls[i]);
    }
    
    snapshotBuffer.push_back(snapshot);
    
    // 限制缓冲区大小（保留最近1秒的快照）
    while (snapshotBuffer.size() > config.updateRate) {
        snapshotBuffer.pop_front();
    }
    
    // 更新远程玩家状态
    for (int i = 0; i < packet->paddleCount; i++) {
        for (auto& player : remotePlayers) {
            if (player.playerId == packet->paddles[i].playerId) {
                player.paddleState = packet->paddles[i];
                player.lastUpdateTime = connectionTimer;
                
                if (onPaddleUpdate) {
                    onPaddleUpdate(player.playerId, player.paddleState);
                }
                break;
            }
        }
    }
}

void NetworkManager::ProcessPaddlePacket(const std::vector<uint8_t>& data) {
    PaddleState* paddleState = (PaddleState*)data.data();
    
    for (auto& player : remotePlayers) {
        if (player.playerId == paddleState->playerId) {
            player.paddleState = *paddleState;
            player.lastUpdateTime = connectionTimer;
            
            if (onPaddleUpdate) {
                onPaddleUpdate(player.playerId, player.paddleState);
            }
            break;
        }
    }
}

void NetworkManager::ProcessBrickDestroyedPacket(const std::vector<uint8_t>& data) {
    BrickDestroyedPacket* packet = (BrickDestroyedPacket*)data.data();
    
    if (onBrickDestroyed) {
        onBrickDestroyed(packet->header.playerId, packet->brickIndex);
    }
}

void NetworkManager::ProcessPowerUpPacket(const std::vector<uint8_t>& data) {
    PowerUpPacket* packet = (PowerUpPacket*)data.data();
    
    if (onPowerUpSpawn) {
        onPowerUpSpawn(packet->header.playerId, packet->powerUpType, 
                      Vector2{packet->position.x, packet->position.y});
    }
}

void NetworkManager::ProcessHostMigrationPacket(const std::vector<uint8_t>& data) {
    NetworkHeader* header = (NetworkHeader*)data.data();
    uint8_t newHostId = header->playerId;
    
    std::cout << "Host migrated to player " << (int)newHostId << std::endl;
    
    if (onHostMigrated) {
        onHostMigrated(newHostId);
    }
}

void NetworkManager::InterpolateGameState(float deltaTime) {
    if (snapshotBuffer.size() < 2) return;
    
    float renderTime = connectionTimer - config.interpolationDelay;
    
    // 找到两个最近的时间快照
    auto it = std::lower_bound(snapshotBuffer.begin(), snapshotBuffer.end(), renderTime,
        [](const SnapshotBuffer& snapshot, float time) {
            return snapshot.timestamp < time;
        });
    
    if (it == snapshotBuffer.end()) {
        // 使用最新快照
        if (onBallUpdate && !snapshotBuffer.empty()) {
            onBallUpdate(snapshotBuffer.back().ballStates);
        }
        return;
    }
    
    if (it == snapshotBuffer.begin()) {
        // 使用最旧快照
        if (onBallUpdate) {
            onBallUpdate(it->ballStates);
        }
        return;
    }
    
    auto prev = it - 1;
    
    // 计算插值因子
    float t = (renderTime - prev->timestamp) / (it->timestamp - prev->timestamp);
    t = std::clamp(t, 0.0f, 1.0f);
    
    // 插值球的状态
    std::vector<BallState> interpolatedBalls;
    size_t ballCount = std::min(prev->ballStates.size(), it->ballStates.size());
    
    for (size_t i = 0; i < ballCount; i++) {
        BallState ball;
        ball.ballId = prev->ballStates[i].ballId;
        ball.position.x = Lerp(prev->ballStates[i].position.x, it->ballStates[i].position.x, t);
        ball.position.y = Lerp(prev->ballStates[i].position.y, it->ballStates[i].position.y, t);
        ball.velocity.x = Lerp(prev->ballStates[i].velocity.x, it->ballStates[i].velocity.x, t);
        ball.velocity.y = Lerp(prev->ballStates[i].velocity.y, it->ballStates[i].velocity.y, t);
        ball.radius = prev->ballStates[i].radius;
        ball.launched = it->ballStates[i].launched;
        
        interpolatedBalls.push_back(ball);
    }
    
    if (onBallUpdate) {
        onBallUpdate(interpolatedBalls);
    }
}

float NetworkManager::Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void NetworkManager::SendPaddleState(const PaddleState& state) {
    if (!host) return;
    
    std::vector<uint8_t> buffer(sizeof(PaddleState));
    PaddleState* packet = reinterpret_cast<PaddleState*>(buffer.data());
    *packet = state;
    
    ENetPacket* enetPacket = enet_packet_create(buffer.data(), buffer.size(),
                                               ENET_PACKET_FLAG_UNSEQUENCED);
    
    if (role == Role::HOST) {
        // 广播给所有客户端
        enet_host_broadcast(host, 1, enetPacket);
    } else {
        // 发送给主机
        if (!remotePlayers.empty() && remotePlayers[0].peer) {
            enet_peer_send(remotePlayers[0].peer, 1, enetPacket);
        }
    }
}

void NetworkManager::SendBallState(const std::vector<BallState>& balls) {
    if (role != Role::HOST || !host) return;
    
    GameStatePacket packet;
    packet.header.messageType = (uint8_t)MessageType::GAME_STATE_UPDATE;
    packet.header.playerId = 0;
    packet.header.sequenceNumber = sequenceNumber++;
    packet.header.timestamp = connectionTimer;
    
    packet.ballCount = std::min(balls.size(), size_t(10));
    for (size_t i = 0; i < packet.ballCount; i++) {
        packet.balls[i] = balls[i];
    }
    
    packet.paddleCount = remotePlayers.size();
    for (size_t i = 0; i < remotePlayers.size(); i++) {
        packet.paddles[i] = remotePlayers[i].paddleState;
    }
    
    ENetPacket* enetPacket = enet_packet_create(&packet, sizeof(GameStatePacket),
                                               ENET_PACKET_FLAG_UNSEQUENCED);
    enet_host_broadcast(host, 0, enetPacket);
}

void NetworkManager::SendBrickDestroyed(uint8_t brickIndex, Vector2 position) {
    if (role != Role::HOST || !host) return;
    
    BrickDestroyedPacket packet;
    packet.header.messageType = (uint8_t)MessageType::BRICK_DESTROYED;
    packet.header.playerId = 0;
    packet.header.sequenceNumber = sequenceNumber++;
    packet.header.timestamp = connectionTimer;
    packet.brickIndex = brickIndex;
    packet.position = {position.x, position.y};
    
    ENetPacket* enetPacket = enet_packet_create(&packet, sizeof(packet),
                                               ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(host, 1, enetPacket);
}

void NetworkManager::SendPowerUpSpawn(uint8_t type, Vector2 position) {
    if (role != Role::HOST || !host) return;
    
    PowerUpPacket packet;
    packet.header.messageType = (uint8_t)MessageType::POWERUP_SPAWN;
    packet.header.playerId = 0;
    packet.header.sequenceNumber = sequenceNumber++;
    packet.header.timestamp = connectionTimer;
    packet.powerUpType = type;
    packet.position = {position.x, position.y};
    
    ENetPacket* enetPacket = enet_packet_create(&packet, sizeof(packet),
                                               ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(host, 1, enetPacket);
}

void NetworkManager::CheckHostMigration() {
    // 检查主机连接状态
    float timeoutThreshold = 5.0f;  // 5秒超时
    
    for (auto it = remotePlayers.begin() + 1; it != remotePlayers.end();) {
        if (connectionTimer - it->lastUpdateTime > timeoutThreshold) {
            std::cout << "Player " << (int)it->playerId << " timed out" << std::endl;
            
            if (onPlayerDisconnected) {
                onPlayerDisconnected(it->playerId);
            }
            
            if (it->peer) {
                enet_peer_disconnect(it->peer, 0);
            }
            
            it = remotePlayers.erase(it);
        } else {
            ++it;
        }
    }
}

void NetworkManager::MigrateToNewHost(uint8_t newHostId) {
    if (role == Role::CLIENT && newHostId == GetLocalPlayerId()) {
        std::cout << "Becoming new host..." << std::endl;
        
        // 转换角色为主机
        role = Role::HOST;
        
        // 重新配置host
        ENetHost* oldHost = host;
        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = config.port;
        
        host = enet_host_create(&address, config.maxPlayers, config.maxChannels,
                               config.incomingBandwidth, config.outgoingBandwidth);
        
        if (host) {
            enet_host_destroy(oldHost);
            
            // 广播主机迁移消息
            NetworkHeader header;
            header.messageType = (uint8_t)MessageType::HOST_MIGRATION;
            header.playerId = newHostId;
            header.sequenceNumber = sequenceNumber++;
            header.timestamp = connectionTimer;
            
            ENetPacket* packet = enet_packet_create(&header, sizeof(header),
                                                   ENET_PACKET_FLAG_RELIABLE);
            enet_host_broadcast(host, 0, packet);
        }
    }
}

void NetworkManager::SimulatePacketLoss(bool enabled, float rate) {
    config.simulatePacketLoss = enabled;
    config.packetLossRate = rate;
}

void NetworkManager::EnablePacketLossSimulation(bool enabled, float rate) {
    SimulatePacketLoss(enabled, rate);
}

float NetworkManager::GetAverageLatency() const {
    if (remotePlayers.empty()) return 0.0f;
    
    float totalLatency = 0.0f;
    int count = 0;
    
    for (const auto& player : remotePlayers) {
        if (player.peer) {
            totalLatency += player.peer->roundTripTime;
            count++;
        }
    }
    
    return count > 0 ? totalLatency / count : 0.0f;
}

void NetworkManager::PrintNetworkStats() const {
    std::cout << "=== Network Statistics ===" << std::endl;
    std::cout << "Role: " << (role == Role::HOST ? "Host" : 
                              role == Role::CLIENT ? "Client" : "None") << std::endl;
    std::cout << "Connected players: " << remotePlayers.size() << std::endl;
    std::cout << "Average latency: " << GetAverageLatency() << "ms" << std::endl;
    std::cout << "Packet loss simulation: " << (config.simulatePacketLoss ? "ON" : "OFF") << std::endl;
    if (config.simulatePacketLoss) {
        std::cout << "Packet loss rate: " << (config.packetLossRate * 100) << "%" << std::endl;
    }
    std::cout << "Snapshot buffer size: " << snapshotBuffer.size() << std::endl;
    std::cout << "==========================" << std::endl;
}

void NetworkManager::SendPong(uint8_t playerId) {
    NetworkHeader header;
    header.messageType = (uint8_t)MessageType::PONG;
    header.playerId = GetLocalPlayerId();
    header.sequenceNumber = sequenceNumber++;
    header.timestamp = connectionTimer;
    
    ENetPacket* packet = enet_packet_create(&header, sizeof(header),
                                           ENET_PACKET_FLAG_UNSEQUENCED);
    
    if (role == Role::HOST) {
        for (auto& player : remotePlayers) {
            if (player.playerId == playerId && player.peer) {
                enet_peer_send(player.peer, 1, packet);
                break;
            }
        }
    } else {
        enet_host_broadcast(host, 1, packet);
    }
}
