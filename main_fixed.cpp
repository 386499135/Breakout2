#include "raylib.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cmath>

#define DEG2RAD (3.14159265358979323846 / 180.0)

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// 简单的粒子结构
struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
};

std::vector<Particle> particles;

void AddParticles(Vector2 pos, Color color) {
    for (int i = 0; i < 10; i++) {
        Particle p;
        p.position = pos;
        p.velocity = {(float)((rand() % 100 - 50) / 10.0f), (float)((rand() % 100 - 50) / 10.0f)};
        p.color = color;
        p.life = 1.0f;
        particles.push_back(p);
    }
}

void UpdateParticles(float deltaTime) {
    for (auto it = particles.begin(); it != particles.end();) {
        it->position.x += it->velocity.x;
        it->position.y += it->velocity.y;
        it->velocity.y += 0.3f;
        it->life -= deltaTime;
        if (it->life <= 0) {
            it = particles.erase(it);
        } else {
            ++it;
        }
    }
    // 限制粒子数量，防止过多
    if (particles.size() > 500) {
        particles.erase(particles.begin(), particles.begin() + 200);
    }
}

void DrawParticles() {
    for (const auto& p : particles) {
        DrawCircleV(p.position, 3, Fade(p.color, p.life));
    }
}

class Brick {
public:
    Rectangle rect;
    bool active;
    Color color;
    
    Brick(float x, float y, float w, float h, Color c) {
        rect = {x, y, w, h};
        active = true;
        color = c;
    }
    
    void Draw() {
        if (active) {
            DrawRectangleRec(rect, color);
            DrawRectangleLinesEx(rect, 1, WHITE);
        }
    }
};

class Ball {
public:
    Vector2 position;
    Vector2 speed;
    float radius;
    bool launched;
    
    Ball(Vector2 pos, Vector2 sp, float r) {
        position = pos;
        speed = sp;
        radius = r;
        launched = false;
    }
    
    void Move() {
        if (!launched) return;
        position.x += speed.x;
        position.y += speed.y;
    }
    
    void Draw() {
        DrawCircleV(position, radius, RED);
        if (!launched) {
            DrawText("PRESS SPACE", position.x - 55, position.y - 30, 16, YELLOW);
        }
    }
    
    void Launch(float paddleX) {
        if (launched) return;
        speed.x = 5.0f;
        speed.y = -7.0f;
        launched = true;
        position.x = paddleX;
        position.y = 550 - radius - 5;
    }
    
    void ResetToPaddle(float paddleX, float paddleY) {
        position.x = paddleX;
        position.y = paddleY - radius - 5;
        speed = {0, 0};
        launched = false;
    }
    
    void ApplyGravity() {
        if (!launched) return;
        speed.y += 0.08f;
        float currentSpeed = sqrt(speed.x * speed.x + speed.y * speed.y);
        if (currentSpeed > 12.0f) {
            speed.x = (speed.x / currentSpeed) * 12.0f;
            speed.y = (speed.y / currentSpeed) * 12.0f;
        }
    }
    
    void BounceEdge() {
        if (!launched) return;
        if (position.x - radius <= 5) {
            position.x = radius + 5;
            speed.x = fabs(speed.x);
        }
        if (position.x + radius >= SCREEN_WIDTH - 5) {
            position.x = SCREEN_WIDTH - radius - 5;
            speed.x = -fabs(speed.x);
        }
        if (position.y - radius <= 5) {
            position.y = radius + 5;
            speed.y = fabs(speed.y);
        }
    }
    
    void BouncePaddle(Rectangle paddleRect) {
        if (!launched) return;
        if (speed.y <= 0) return;
        
        if (position.y + radius >= paddleRect.y &&
            position.y + radius <= paddleRect.y + paddleRect.height + fabs(speed.y) &&
            position.x >= paddleRect.x - radius &&
            position.x <= paddleRect.x + paddleRect.width + radius) {
            
            float hitPoint = (position.x - (paddleRect.x + paddleRect.width / 2.0f)) / (paddleRect.width / 2.0f);
            hitPoint = std::clamp(hitPoint, -1.0f, 1.0f);
            
            float speedMagnitude = sqrt(speed.x * speed.x + speed.y * speed.y);
            speedMagnitude = std::max(speedMagnitude, 6.0f);
            
            float angle = 90.0f - hitPoint * 50.0f;
            float angleRad = angle * 3.14159f / 180.0f;
            
            speed.x = speedMagnitude * cos(angleRad);
            speed.y = -speedMagnitude * fabs(sin(angleRad));
            position.y = paddleRect.y - radius;
        }
    }
    
    bool CheckBrickCollision(Rectangle brickRect) {
        if (!launched) return false;
        
        float closestX = std::max(brickRect.x, std::min(position.x, brickRect.x + brickRect.width));
        float closestY = std::max(brickRect.y, std::min(position.y, brickRect.y + brickRect.height));
        
        float distX = position.x - closestX;
        float distY = position.y - closestY;
        float distance = sqrt(distX * distX + distY * distY);
        
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
    
    Vector2 GetPosition() const { return position; }
    bool IsLaunched() const { return launched; }
};

class Paddle {
public:
    Rectangle rect;
    
    Paddle(float x, float y, float width, float height) {
        rect = {x, y, width, height};
    }
    
    void MoveLeft(float speed) {
        rect.x -= speed;
        if (rect.x < 5) rect.x = 5;
    }
    
    void MoveRight(float speed) {
        rect.x += speed;
        if (rect.x + rect.width > SCREEN_WIDTH - 5) rect.x = SCREEN_WIDTH - rect.width - 5;
    }
    
    void Draw() {
        DrawRectangleRec(rect, BLUE);
        DrawRectangleLinesEx(rect, 2, SKYBLUE);
    }
    
    Rectangle GetRect() const { return rect; }
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Breakout");
    SetTargetFPS(60);
    srand(time(nullptr));
    
    // 初始化砖块
    std::vector<Brick> bricks;
    Color colors[] = {RED, ORANGE, YELLOW, GREEN, BLUE};
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 8; col++) {
            bricks.emplace_back(50.0f + col * 95, 80.0f + row * 35, 85.0f, 25.0f, colors[row]);
        }
    }
    
    // 初始化球和挡板
    std::vector<Ball> balls;
    balls.emplace_back(Vector2{400, 530}, Vector2{0, 0}, 10);
    Paddle paddle(340, 550, 120, 15);
    
    int score = 0;
    int lives = 5;
    int winCount = bricks.size();
    bool gameOver = false;
    bool victory = false;
    bool anyBallLaunched = false;
    float gameSpeed = 1.0f;
    float splitCooldown = 0.0f;
    
    while (!WindowShouldClose() && !gameOver) {
        float deltaTime = GetFrameTime();
        deltaTime = std::min(deltaTime, 0.033f); // 限制最大delta时间
        
        // 更新冷却
        if (splitCooldown > 0) splitCooldown -= deltaTime;
        
        // 输入
        if (IsKeyPressed(KEY_R)) {
            // 重置游戏
            balls.clear();
            balls.emplace_back(Vector2{400, 530}, Vector2{0, 0}, 10);
            paddle = Paddle(340, 550, 120, 15);
            bricks.clear();
            for (int row = 0; row < 5; row++) {
                for (int col = 0; col < 8; col++) {
                    bricks.emplace_back(50.0f + col * 95, 80.0f + row * 35, 85.0f, 25.0f, colors[row]);
                }
            }
            score = 0;
            lives = 5;
            winCount = bricks.size();
            anyBallLaunched = false;
            particles.clear();
            gameSpeed = 1.0f;
            continue;
        }
        
        if (IsKeyPressed(KEY_S) && splitCooldown <= 0 && !balls.empty()) {
            // 分裂球
            std::vector<Ball> newBalls;
            for (auto& ball : balls) {
                if (ball.IsLaunched()) {
                    Ball newBall(ball.GetPosition(), {ball.speed.x * 0.8f, ball.speed.y * 0.8f}, ball.radius * 0.7f);
                    newBall.launched = true;
                    newBalls.push_back(newBall);
                }
            }
            for (auto& b : newBalls) {
                balls.push_back(b);
            }
            splitCooldown = 2.0f;
        }
        
        // 挡板移动
        float currentSpeed = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) ? 28.0f : 18.0f;
        currentSpeed *= gameSpeed;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) paddle.MoveLeft(currentSpeed);
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) paddle.MoveRight(currentSpeed);
        
        // 球管理
        anyBallLaunched = false;
        for (auto& ball : balls) {
            if (!ball.IsLaunched()) {
                ball.ResetToPaddle(paddle.GetRect().x + paddle.GetRect().width / 2, paddle.GetRect().y);
            } else {
                anyBallLaunched = true;
            }
        }
        
        if (!anyBallLaunched && IsKeyPressed(KEY_SPACE)) {
            balls[0].Launch(paddle.GetRect().x + paddle.GetRect().width / 2);
        }
        
        // 更新球
        for (auto& ball : balls) {
            if (!ball.IsLaunched()) continue;
            
            ball.ApplyGravity();
            ball.Move();
            ball.BounceEdge();
            ball.BouncePaddle(paddle.GetRect());
            
            for (auto& brick : bricks) {
                if (brick.active && ball.CheckBrickCollision(brick.rect)) {
                    brick.active = false;
                    score += 10;
                    winCount--;
                    AddParticles({brick.rect.x + 42, brick.rect.y + 12}, brick.color);
                    break;
                }
            }
        }
        
        // 移除出界球
        balls.erase(std::remove_if(balls.begin(), balls.end(),
            [](Ball& ball) { return ball.GetPosition().y > SCREEN_HEIGHT + 50; }), balls.end());
        
        // 生命损失
        if (balls.empty()) {
            lives--;
            if (lives <= 0) {
                gameOver = true;
                victory = false;
            } else {
                balls.emplace_back(Vector2{400, 530}, Vector2{0, 0}, 10);
                anyBallLaunched = false;
            }
        }
        
        // 通关
        if (winCount <= 0) {
            gameOver = true;
            victory = true;
        }
        
        // 更新粒子
        UpdateParticles(deltaTime);
        
        // 渲染
        BeginDrawing();
        ClearBackground(BLACK);
        
        // 边框
        DrawRectangle(0, 0, 5, SCREEN_HEIGHT, GRAY);
        DrawRectangle(SCREEN_WIDTH - 5, 0, 5, SCREEN_HEIGHT, GRAY);
        DrawRectangle(0, 0, SCREEN_WIDTH, 5, GRAY);
        
        // 游戏对象
        for (auto& brick : bricks) brick.Draw();
        paddle.Draw();
        for (auto& ball : balls) ball.Draw();
        DrawParticles();
        
        // UI
        DrawText(TextFormat("Score: %d", score), 20, 10, 20, WHITE);
        DrawText(TextFormat("FPS: %d", GetFPS()), SCREEN_WIDTH - 80, 10, 20, LIME);
        DrawText(TextFormat("Lives: %d", lives), 20, 35, 20, lives > 3 ? GREEN : (lives > 1 ? YELLOW : RED));
        DrawText(TextFormat("Balls: %d", (int)balls.size()), 20, 60, 20, WHITE);
        
        if (splitCooldown > 0) {
            DrawText(TextFormat("Split CD: %.1fs", splitCooldown), 20, 85, 14, RED);
        }
        
        DrawText("S:Split | R:Reset", 20, SCREEN_HEIGHT - 25, 14, Fade(WHITE, 0.5f));
        
        if (!anyBallLaunched && !gameOver) {
            float alpha = (sinf(GetTime() * 4.0f) + 1.0f) * 0.3f + 0.4f;
            DrawText("PRESS SPACE TO LAUNCH", SCREEN_WIDTH/2 - 120, 400, 20, Fade(YELLOW, alpha));
        }
        
        EndDrawing();
    }
    
    // 游戏结束画面
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        if (victory) {
            DrawText("VICTORY!", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 30, 40, GREEN);
        } else {
            DrawText("GAME OVER", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 30, 40, RED);
        }
        DrawText(TextFormat("Final Score: %d", score), SCREEN_WIDTH/2 - 70, SCREEN_HEIGHT/2 + 20, 28, YELLOW);
        DrawText("Press R to Restart", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 80, 20, WHITE);
        DrawText("Press ESC to Exit", SCREEN_WIDTH/2 - 70, SCREEN_HEIGHT/2 + 120, 16, WHITE);
        
        if (IsKeyPressed(KEY_R)) {
            // 重新启动游戏
            main();
            return 0;
        }
        if (IsKeyPressed(KEY_ESCAPE)) break;
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}
