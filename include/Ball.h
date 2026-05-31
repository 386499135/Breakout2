#ifndef BALL_H
#define BALL_H

#include "raylib.h"
#include <vector>

class Ball {
private:
    Vector2 position;
    Vector2 speed;
    float radius;
    float gravity;
    float maxSpeed;
    bool launched;
    
public:
    Ball(Vector2 pos = {400, 530}, Vector2 sp = {0, 0}, float r = 10.0f);
    
    void Move();
    void Draw();
    void ApplyGravity();
    void BounceEdge();
    void BouncePaddle(Rectangle paddleRect);
    bool CheckBrickCollision(Rectangle brickRect);
    
    void Launch(float paddleX);
    void ResetToPaddle(float paddleX, float paddleY);
    
    std::vector<Ball> Split(int count = 2);
    
    Vector2 GetPosition() const { return position; }
    Vector2 GetSpeed() const { return speed; }
    float GetRadius() const { return radius; }
    bool IsLaunched() const { return launched; }
    void SetLaunched(bool l) { launched = l; }
    void SetSpeed(Vector2 sp) { speed = sp; }
    void SetPosition(Vector2 pos) { position = pos; }
};

#endif
