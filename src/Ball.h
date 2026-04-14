#ifndef BALL_H
#define BALL_H
#include "raylib.h"
#include <cmath>

class Ball {
private:
    Vector2 position;
    Vector2 speed;
    float radius;
    float gravity;
    float maxSpeed;
    float bounceForce;
    bool launched;
    float launchCooldown;
    
public:
    Ball(Vector2 pos, Vector2 sp, float r);
    
    void Move();
    void Draw();
    void ApplyGravity();
    void BounceEdge(int screenWidth, int screenHeight);
    void BouncePaddle(Rectangle paddleRect);
    bool CheckBrickCollision(Rectangle brickRect);
    
    void Launch(float paddleX, float paddleWidth);
    void ResetToPaddle(float paddleX, float paddleY);
    void Reset(Vector2 pos, Vector2 sp);
    void AddBounceForce(float force);
    
    Vector2 GetPosition() const { return position; }
    float GetRadius() const { return radius; }
    Vector2 GetSpeed() const { return speed; }
    void SetSpeed(Vector2 sp) { speed = sp; }
    void SetPosition(Vector2 pos) { position = pos; }
    void SetLaunched(bool l) { launched = l; }
    bool IsLaunched() const { return launched; }
};

#endif
