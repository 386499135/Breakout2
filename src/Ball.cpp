#include "Ball.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


Ball::Ball(Vector2 pos, Vector2 sp, float r) {
    position = pos;
    speed = sp;
    radius = r;
    gravity = 0.08f;
    maxSpeed = 15.0f;
    bounceForce = 0.5f;
    launched = false;
    launchCooldown = 0.0f;
    
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(nullptr));
        seeded = true;
    }
}

void Ball::Move() {
    if (!launched) return;
    position.x += speed.x;
    position.y += speed.y;
}

void Ball::Draw() {
    DrawCircleGradient((int)position.x, (int)position.y, radius + 3, Fade(ORANGE, 0.3f), RED);
    DrawCircleV(position, radius, RED);
    
    if (launched) {
        Vector2 endPos = { position.x + speed.x * 2, position.y + speed.y * 2 };
        DrawLineEx(position, endPos, 2, YELLOW);
    } else {
        if ((int)(GetTime() * 2) % 2 == 0) {
            DrawText("PRESS SPACE", (int)position.x - 55, (int)position.y - 30, 16, YELLOW);
        }
    }
}

void Ball::Launch(float paddleX, float paddleWidth) {
    if (launched) return;
    
    float angle = (rand() % 61 + 60) * 3.14159f / 180.0f;
    if (rand() % 2 == 0) angle = 3.14159f - angle;
    
    float baseSpeed = 8.0f;
    float speedVariation = (rand() % 5 - 2) * 0.5f;
    float launchSpeed = baseSpeed + speedVariation;
    
    speed.x = launchSpeed * std::cos(angle);
    speed.y = -launchSpeed * std::abs(std::sin(angle));
    
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

void Ball::Reset(Vector2 pos, Vector2 sp) {
    position = pos;
    speed = sp;
}

void Ball::AddBounceForce(float force) {
    speed.y -= force;
}

void Ball::ApplyGravity() {
    if (!launched) return;
    speed.y += gravity;
    
    float currentSpeed = std::sqrt(speed.x * speed.x + speed.y * speed.y);
    if (currentSpeed > maxSpeed) {
        speed.x = (speed.x / currentSpeed) * maxSpeed;
        speed.y = (speed.y / currentSpeed) * maxSpeed;
    }
}

void Ball::BounceEdge(int screenWidth, int screenHeight) {
    if (!launched) return;
    
    if (position.x - radius <= 5) {
        position.x = radius + 5;
        speed.x = std::abs(speed.x);
    }
    if (position.x + radius >= screenWidth - 5) {
        position.x = screenWidth - radius - 5;
        speed.x = -std::abs(speed.x);
    }
    if (position.y - radius <= 5) {
        position.y = radius + 5;
        speed.y = std::abs(speed.y);
        speed.y += bounceForce;
    }
}

void Ball::BouncePaddle(Rectangle paddleRect) {
    if (!launched) return;
    if (speed.y <= 0) return;
    
    if (position.y + radius >= paddleRect.y &&
        position.y + radius <= paddleRect.y + paddleRect.height + std::abs(speed.y) &&
        position.x >= paddleRect.x - radius &&
        position.x <= paddleRect.x + paddleRect.width + radius) {
        
        float hitPoint = (position.x - (paddleRect.x + paddleRect.width / 2.0f)) / (paddleRect.width / 2.0f);
        hitPoint = std::clamp(hitPoint, -1.0f, 1.0f);
        
        float speedMagnitude = std::sqrt(speed.x * speed.x + speed.y * speed.y);
        speedMagnitude = std::max(speedMagnitude + bounceForce * 2, 7.0f);
        
        float angle = 90.0f - hitPoint * 50.0f;
        float angleRad = angle * 3.14159f / 180.0f;
        
        speed.x = speedMagnitude * std::cos(angleRad);
        speed.y = -speedMagnitude * std::abs(std::sin(angleRad));
        position.y = paddleRect.y - radius;
    }
}

bool Ball::CheckBrickCollision(Rectangle brickRect) {
    if (!launched) return false;
    
    float closestX = std::max(brickRect.x, std::min(position.x, brickRect.x + brickRect.width));
    float closestY = std::max(brickRect.y, std::min(position.y, brickRect.y + brickRect.height));
    
    float distX = position.x - closestX;
    float distY = position.y - closestY;
    float distance = std::sqrt(distX * distX + distY * distY);
    
    if (distance < radius) {
        float distLeft = position.x - brickRect.x;
        float distRight = brickRect.x + brickRect.width - position.x;
        float distTop = position.y - brickRect.y;
        float distBottom = brickRect.y + brickRect.height - position.y;
        
        float minDistX = std::min(distLeft, distRight);
        float minDistY = std::min(distTop, distBottom);
        
        if (minDistX < minDistY) {
            speed.x *= -1;
            position.x = (distLeft < distRight) ? brickRect.x - radius : brickRect.x + brickRect.width + radius;
        } else {
            speed.y *= -1;
            if (distTop > distBottom) speed.y -= bounceForce;
            position.y = (distTop < distBottom) ? brickRect.y - radius : brickRect.y + brickRect.height + radius;
        }
        return true;
    }
    return false;
}

// ============== 分裂功能实现 ==============
std::vector<Ball> Ball::Split(int count) {
    std::vector<Ball> newBalls;
    
    if (!launched) return newBalls;
    
    // 减小当前球的半径
    radius = radius * 0.7f;
    if (radius < 5.0f) radius = 5.0f;
    
    // 获取当前速度大小
    float currentSpeed = std::sqrt(speed.x * speed.x + speed.y * speed.y);
    float splitSpeed = std::max(currentSpeed, 6.0f);
    
    // 创建新球，向不同方向发射
    for (int i = 0; i < count; i++) {
        float angle = ((float)i / count) * 2.0f * M_PI + ((float)(rand() % 30) * M_PI / 180.0f);
        
        Vector2 newSpeed;
        newSpeed.x = std::cos(angle) * splitSpeed;
        newSpeed.y = std::sin(angle) * splitSpeed;
        
        // 确保新球不会完全水平或垂直
        if (std::abs(newSpeed.x) < 1.0f) newSpeed.x = (newSpeed.x >= 0 ? 1.0f : -1.0f) * 2.0f;
        if (std::abs(newSpeed.y) < 1.0f) newSpeed.y = -3.0f;
        
        Ball newBall(position, newSpeed, radius);
        newBall.SetLaunched(true);
        newBalls.push_back(newBall);
    }
    
    // 给当前球也加一点随机偏移，避免所有球轨迹相同
    float randomAngle = ((float)(rand() % 60 - 30) * M_PI / 180.0f);
    float cosA = std::cos(randomAngle);
    float sinA = std::sin(randomAngle);
    float newX = speed.x * cosA - speed.y * sinA;
    float newY = speed.x * sinA + speed.y * cosA;
    speed.x = newX;
    speed.y = newY;
    
    return newBalls;
}

void Ball::SetRandomDirection(float baseSpeed) {
    if (!launched) return;
    
    float angle = ((float)(rand() % 360) * M_PI / 180.0f);
    speed.x = std::cos(angle) * baseSpeed;
    speed.y = std::sin(angle) * baseSpeed;
    
    // 确保有足够的垂直速度
    if (std::abs(speed.y) < 2.0f) {
        speed.y = (speed.y >= 0 ? 2.0f : -2.0f) * baseSpeed * 0.5f;
    }
}
