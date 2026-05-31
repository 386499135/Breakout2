#include "Ball.h"
#include "GameConstants.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Ball::Ball(Vector2 pos, Vector2 sp, float r) {
    position = pos;
    speed = sp;
    radius = r;
    gravity = 0.06f;
    maxSpeed = 18.0f;
    launched = false;
}

void Ball::Move() {
    if (!launched) return;
    position.x += speed.x;
    position.y += speed.y;
}

void Ball::Draw() {
    for (int i = 1; i <= 3; i++) {
        DrawCircleV(position, radius + i, Fade(RED, 0.1f));
    }
    DrawCircleV(position, radius, RED);
    DrawCircleGradient(position.x, position.y, radius - 2, ORANGE, RED);
    
    if (!launched) {
        float alpha = (sinf(GetTime() * 4.0f) + 1.0f) * 0.3f + 0.4f;
        DrawText("PRESS SPACE", position.x - 55, position.y - 30, 16, Fade(YELLOW, alpha));
    }
}

void Ball::ApplyGravity() {
    if (!launched) return;
    speed.y += gravity;
    
    float currentSpeed = sqrtf(speed.x * speed.x + speed.y * speed.y);
    if (currentSpeed > maxSpeed) {
        speed.x = (speed.x / currentSpeed) * maxSpeed;
        speed.y = (speed.y / currentSpeed) * maxSpeed;
    }
}

void Ball::BounceEdge() {
    if (!launched) return;
    
    if (position.x - radius <= 5) {
        position.x = radius + 5;
        speed.x = fabsf(speed.x);
    }
    if (position.x + radius >= SCREEN_WIDTH - 5) {
        position.x = SCREEN_WIDTH - radius - 5;
        speed.x = -fabsf(speed.x);
    }
    if (position.y - radius <= 5) {
        position.y = radius + 5;
        speed.y = fabsf(speed.y);
    }
}

void Ball::BouncePaddle(Rectangle paddleRect) {
    if (!launched) return;
    if (speed.y <= 0) return;
    
    if (position.y + radius >= paddleRect.y &&
        position.y + radius <= paddleRect.y + paddleRect.height + fabsf(speed.y) &&
        position.x >= paddleRect.x - radius &&
        position.x <= paddleRect.x + paddleRect.width + radius) {
        
        float hitPoint = (position.x - (paddleRect.x + paddleRect.width / 2.0f)) / (paddleRect.width / 2.0f);
        hitPoint = std::clamp(hitPoint, -1.0f, 1.0f);
        
        float speedMagnitude = sqrtf(speed.x * speed.x + speed.y * speed.y);
        speedMagnitude = std::max(speedMagnitude, 7.0f);
        
        float angle = 90.0f - hitPoint * 55.0f;
        float angleRad = angle * M_PI / 180.0f;
        
        speed.x = speedMagnitude * cosf(angleRad);
        speed.y = -speedMagnitude * fabsf(sinf(angleRad));
        position.y = paddleRect.y - radius;
    }
}

bool Ball::CheckBrickCollision(Rectangle brickRect) {
    if (!launched) return false;
    
    float closestX = std::max(brickRect.x, std::min(position.x, brickRect.x + brickRect.width));
    float closestY = std::max(brickRect.y, std::min(position.y, brickRect.y + brickRect.height));
    
    float distX = position.x - closestX;
    float distY = position.y - closestY;
    float distance = sqrtf(distX * distX + distY * distY);
    
    if (distance < radius) {
        float distLeft = position.x - brickRect.x;
        float distRight = brickRect.x + brickRect.width - position.x;
        float distTop = position.y - brickRect.y;
        float distBottom = brickRect.y + brickRect.height - position.y;
        
        if (std::min(distLeft, distRight) < std::min(distTop, distBottom)) {
            speed.x *= -1;
        } else {
            speed.y *= -1;
        }
        return true;
    }
    return false;
}

void Ball::Launch(float paddleX) {
    if (launched) return;
    speed.x = 5.5f;
    speed.y = -7.5f;
    launched = true;
    position.x = paddleX;
    position.y = 550 - radius - 5;
}

void Ball::ResetToPaddle(float paddleX, float paddleY) {
    position.x = paddleX;
    position.y = paddleY - radius - 5;
    speed = {0, 0};
    launched = false;
}

std::vector<Ball> Ball::Split(int count) {
    std::vector<Ball> newBalls;
    if (!launched) return newBalls;
    
    radius = radius * 0.7f;
    if (radius < 4.0f) radius = 4.0f;
    
    float currentSpeed = sqrtf(speed.x * speed.x + speed.y * speed.y);
    float splitSpeed = std::max(currentSpeed, 7.0f);
    
    for (int i = 0; i < count; i++) {
        float angle = ((float)i / count) * 2.0f * M_PI + (rand() % 30) * M_PI / 180.0f;
        Vector2 newSpeed;
        newSpeed.x = cosf(angle) * splitSpeed;
        newSpeed.y = sinf(angle) * splitSpeed;
        
        Ball newBall(position, newSpeed, radius);
        newBall.SetLaunched(true);
        newBalls.push_back(newBall);
    }
    
    return newBalls;
}
