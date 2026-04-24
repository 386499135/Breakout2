#ifndef NETWORK_PROTOCOL_H
#define NETWORK_PROTOCOL_H

#include <cstdint>
#include <cstring>

// 网络消息类型
enum class MessageType : uint8_t {
    // 连接管理
    CLIENT_CONNECT = 0x01,
    CLIENT_DISCONNECT = 0x02,
    SERVER_FULL = 0x03,
    CONNECTION_ACCEPTED = 0x04,
    
    // 状态同步
    GAME_STATE_UPDATE = 0x10,
    PADDLE_POSITION = 0x11,
    BALL_STATE = 0x12,
    
    // 游戏事件
    BALL_LAUNCH = 0x20,
    BRICK_DESTROYED = 0x21,
    POWERUP_SPAWN = 0x22,
    POWERUP_COLLECTED = 0x23,
    LIFE_LOST = 0x24,
    
    // 控制消息
    HOST_MIGRATION = 0x30,
    PING = 0x40,
    PONG = 0x41
};

// 网络数据包结构
#pragma pack(push, 1)

struct NetworkHeader {
    uint8_t messageType;
    uint8_t playerId;
    uint16_t sequenceNumber;
    float timestamp;
};

struct Vector2Network {
    float x;
    float y;
};

struct PaddleState {
    uint8_t playerId;
    Vector2Network position;
    float width;
};

struct BallState {
    uint8_t ballId;
    Vector2Network position;
    Vector2Network velocity;
    float radius;
    bool launched;
};

struct GameStatePacket {
    NetworkHeader header;
    uint8_t ballCount;
    BallState balls[10];  // 最多10个球
    uint8_t paddleCount;
    PaddleState paddles[4]; // 最多4个玩家
    int32_t score;
    uint8_t lives;
    float gameSpeed;
};

struct BrickDestroyedPacket {
    NetworkHeader header;
    uint8_t brickIndex;
    Vector2Network position;
};

struct PowerUpPacket {
    NetworkHeader header;
    uint8_t powerUpType;
    Vector2Network position;
};

#pragma pack(pop)

#endif
