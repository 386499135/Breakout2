#ifndef PADDLE_H
#define PADDLE_H
#include "raylib.h"
#include <cstdint>

class Paddle {
private:
    Rectangle rect;
    float screenWidth;
    uint8_t playerId;  // 新增：玩家ID
    Color color;       // 新增：玩家颜色

public:
    Paddle(float x, float y, float width, float height);
    void MoveLeft(float speed);
    void MoveRight(float speed);
    void Draw();
    Rectangle GetRect() const { return rect; }
    void SetRect(Rectangle r) { rect = r; }

    void SetPlayerId(uint8_t id) { playerId = id; }
    uint8_t GetPlayerId() const { return playerId; }
    void SetColor(Color c) { color = c; }
    Color GetColor() const { return color; }
    void SetPosition(Vector2 pos) { rect.x = pos.x; rect.y = pos.y; }
};

#endif
