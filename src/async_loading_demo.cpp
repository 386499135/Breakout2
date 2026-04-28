#include "raylib.h"
#include "AsyncLoader.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "PowerUp.h"
#include <vector>
#include <iostream>
#include <mutex>
#include <future>
#include <atomic>
#include <thread>
#include <cmath>
#include <sstream>
#include <algorithm>

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    
    InitWindow(screenWidth, screenHeight, "Breakout - Ball Split Demo");
    SetTargetFPS(60);
    
    // 游戏对象
    std::vector<Ball> balls;
    balls.emplace_back(Vector2{400.0f, 530.0f}, Vector2{0.0f, 0.0f}, 10.0f);
    Paddle paddle(340.0f, 550.0f, 120.0f, 15.0f);
    
    // 砖块
    std::vector<Brick> bricks;
    Color originalBrickColors[] = {RED, ORANGE, YELLOW, GREEN, BLUE};
    
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 8; col++) {
            if (rand() % 10 < 8) {  // 80% 概率有砖块
                bricks.emplace_back(50.0f + col * 95, 80.0f + row * 35, 85.0f, 25.0f, originalBrickColors[row]);
            }
        }
    }
    
    // 道具系统
    PowerUpFactory powerUpFactory;
    std::vector<std::unique_ptr<PowerUp>> powerUps;
    ParticleSystem particleSystem;
    
    // 游戏状态
    int score = 0;
    int lives = 5;
    int winCount = (int)bricks.size();
    bool gameOver = false;
    bool paused = false;
    bool victory = false;
    float gameTime = 0.0f;
    float gameSpeed = 1.0f;
    bool anyBallLaunched = false;
    
    GameState gameState(&balls, &paddle, &lives, &score, &gameSpeed);
    
    // 异步加载器
    AsyncLoader asyncLoader;
    
    // 线程安全数据
    std::mutex colorMutex;
    std::vector<Color> newColors;
    std::atomic<bool> colorsReady{false};
    bool loadCompleted = false;
    float completionTimer = 0.0f;
    bool brickColorsChanged = false;
    
    // 道具效果计时器
    std::vector<std::pair<std::unique_ptr<PowerUpEffect>, float>> activeEffects;
    
    // 随机数
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dropDist(0.0f, 1.0f);
    
    // 分裂冷却时间
    float splitCooldown = 0.0f;
    const float SPLIT_COOLDOWN_TIME = 2.0f;
    
    std::ostringstream tidStr;
    tidStr << std::this_thread::get_id();
    std::string mainThreadId = tidStr.str();
    
    std::cout << "========================================" << std::endl;
    std::cout << "Breakout - Ball Split Demo" << std::endl;
    std::cout << "SPACE: Launch | S: Split balls" << std::endl;
    std::cout << "L: Async load | P: Pause | R: Reset" << std::endl;
    std::cout << "========================================" << std::endl;
    
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        asyncLoader.Update();
        
        // 更新冷却时间
        if (splitCooldown > 0) splitCooldown -= deltaTime;
        
        // ============== 输入处理 ==============
        if (IsKeyPressed(KEY_P) && !gameOver) paused = !paused;
        
        // S 键：分裂球
        if (IsKeyPressed(KEY_S) && !gameOver && !paused && splitCooldown <= 0) {
            std::vector<Ball> newBalls;
            for (auto& ball : balls) {
                if (ball.IsLaunched()) {
                    auto splits = ball.Split(2);  // 每个球分裂成 2 个
                    for (auto& b : splits) {
                        newBalls.push_back(b);
                    }
                }
            }
            
            // 添加新球
            for (auto& b : newBalls) {
                balls.push_back(std::move(b));
            }
            
            splitCooldown = SPLIT_COOLDOWN_TIME;
            std::cout << "[Game] Ball split! Total balls: " << balls.size() << std::endl;
        }
        
        // L 键：异步加载
        if (IsKeyPressed(KEY_L) && !asyncLoader.IsLoading() && !gameOver) {
            std::cout << "\n[Main Thread] >>> Async load started <<<" << std::endl;
            
            asyncLoader.StartSimulatedLoad(3.0f);
            
            static std::future<void> storedFuture;
            storedFuture = std::async(std::launch::async, 
                [&newColors, &colorsReady, &colorMutex]() {
                std::vector<Color> generated;
                for (int i = 0; i < 40; i++) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(40));
                    Color c;
                    c.r = (unsigned char)((sinf(i * 0.3f) * 127 + 128));
                    c.g = (unsigned char)((sinf(i * 0.5f) * 127 + 128));
                    c.b = (unsigned char)((sinf(i * 0.7f) * 127 + 128));
                    c.a = 255;
                    generated.push_back(c);
                }
                {
                    std::lock_guard<std::mutex> lock(colorMutex);
                    newColors = std::move(generated);
                    colorsReady.store(true);
                }
            });
        }
        
        if (IsKeyPressed(KEY_R)) {
            balls.clear();
            balls.emplace_back(Vector2{400.0f, 530.0f}, Vector2{0.0f, 0.0f}, 10.0f);
            paddle = Paddle(340.0f, 550.0f, 120.0f, 15.0f);
            powerUps.clear();
            activeEffects.clear();
            particleSystem.Clear();
            score = 0;
            lives = 5;
            gameOver = false;
            victory = false;
            paused = false;
            gameTime = 0.0f;
            gameSpeed = 1.0f;
            loadCompleted = false;
            completionTimer = 0.0f;
            brickColorsChanged = false;
            splitCooldown = 0.0f;
            
            bricks.clear();
            for (int row = 0; row < 5; row++) {
                for (int col = 0; col < 8; col++) {
                    if (rand() % 10 < 8) {
                        bricks.emplace_back(50.0f + col * 95, 80.0f + row * 35, 85.0f, 25.0f, originalBrickColors[row]);
                    }
                }
            }
            winCount = (int)bricks.size();
            asyncLoader.Reset();
            colorsReady.store(false);
            std::cout << "[Game] Reset" << std::endl;
        }
        
        // ============== 游戏逻辑 ==============
        if (!gameOver && !paused) {
            gameTime += deltaTime;
            
            // 挡板
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
                balls[0].Launch(paddle.GetRect().x + paddle.GetRect().width / 2, paddle.GetRect().width);
            }
            
            // 更新球
            for (auto& ball : balls) {
                if (!ball.IsLaunched()) continue;
                
                ball.ApplyGravity();
                ball.Move();
                ball.BounceEdge(screenWidth, screenHeight);
                ball.BouncePaddle(paddle.GetRect());
                
                for (auto& brick : bricks) {
                    if (brick.IsActive() && ball.CheckBrickCollision(brick.GetRect())) {
                        brick.SetActive(false);
                        score += 10;
                        winCount--;
                        
                        Vector2 brickCenter = {
                            brick.GetRect().x + brick.GetRect().width / 2,
                            brick.GetRect().y + brick.GetRect().height / 2
                        };
                        particleSystem.EmitBrickBreak(brickCenter, brick.GetColor(), 15);
                        
                        // 道具掉落
                        if (dropDist(gen) < powerUpFactory.GetDropChance()) {
                            auto powerUp = powerUpFactory.CreateRandomPowerUp(brickCenter);
                            if (powerUp) {
                                powerUps.push_back(std::move(powerUp));
                            }
                        }
                        break;
                    }
                }
            }
            
            // 移除出界的球
            balls.erase(std::remove_if(balls.begin(), balls.end(),
                [screenHeight](Ball& ball) {
                    return ball.GetPosition().y > screenHeight + 50;
                }), balls.end());
            
            if (balls.empty()) {
                lives--;
                if (lives <= 0) {
                    gameOver = true;
                } else {
                    Ball newBall(Vector2{400.0f, 530.0f}, Vector2{0.0f, 0.0f}, 10.0f);
                    balls.push_back(newBall);
                }
            }
            
            if (winCount <= 0) {
                gameOver = true;
                victory = true;
            }
            
            // 更新道具
            for (auto it = powerUps.begin(); it != powerUps.end();) {
                (*it)->Update(deltaTime);
                if ((*it)->CheckCollision(paddle.GetRect())) {
                    (*it)->Apply(&gameState);
                    it = powerUps.erase(it);
                } else if (!(*it)->IsActive()) {
                    it = powerUps.erase(it);
                } else {
                    ++it;
                }
            }
            
            // 管理道具效果计时
            for (auto it = activeEffects.begin(); it != activeEffects.end();) {
                it->second -= deltaTime;
                if (it->second <= 0) {
                    it->first->Remove(&gameState);
                    it = activeEffects.erase(it);
                } else {
                    ++it;
                }
            }
            
            particleSystem.Update();
        }
        
        // 应用异步加载结果
        if (!asyncLoader.IsLoading() && asyncLoader.GetState() == LoadState::COMPLETED) {
            if (!loadCompleted) {
                loadCompleted = true;
                completionTimer = 0.0f;
                
                if (colorsReady.load()) {
                    std::lock_guard<std::mutex> lock(colorMutex);
                    if (!newColors.empty()) {
                        int brickIndex = 0;
                        for (auto& brick : bricks) {
                            if (brick.IsActive() && brickIndex < (int)newColors.size()) {
                                Rectangle rect = brick.GetRect();
                                bricks[brickIndex] = Brick(rect.x, rect.y, rect.width, rect.height, newColors[brickIndex]);
                                brickIndex++;
                            }
                        }
                        brickColorsChanged = true;
                        colorsReady.store(false);
                        std::cout << "[Main Thread] Colors changed!" << std::endl;
                    }
                }
            }
        }
        
        if (loadCompleted) completionTimer += deltaTime;
        
        // ============== 渲染 ==============
        BeginDrawing();
        ClearBackground(Color{20, 25, 35, 255});
        
        // 边框
        DrawRectangle(0, 0, 5, screenHeight, Color{60, 60, 80, 255});
        DrawRectangle(screenWidth - 5, 0, 5, screenHeight, Color{60, 60, 80, 255});
        DrawRectangle(0, 0, screenWidth, 5, Color{60, 60, 80, 255});
        
        // 游戏对象
        for (auto& brick : bricks) brick.Draw();
        paddle.Draw();
        for (auto& ball : balls) ball.Draw();
        for (auto& powerUp : powerUps) powerUp->Draw(particleSystem);
        particleSystem.Draw();
        
        // HUD
        DrawText(TextFormat("Score: %d", score), 20, 10, 20, WHITE);
        DrawText(TextFormat("Lives: %d", lives), 20, 35, 20,
                 lives > 3 ? GREEN : (lives > 1 ? YELLOW : RED));
        DrawText(TextFormat("Balls: %d", (int)balls.size()), 20, 60, 20,
                 balls.size() > 3 ? PURPLE : Fade(WHITE, 0.7f));
        DrawText(TextFormat("Speed: %.1fx", gameSpeed), 20, 85, 16,
                 gameSpeed != 1.0f ? YELLOW : Fade(WHITE, 0.5f));
        
        // 分裂冷却
        if (splitCooldown > 0) {
            DrawText(TextFormat("Split CD: %.1fs", splitCooldown), 20, 105, 14, Fade(RED, 0.7f));
        }
        
        // 提示
        if (!anyBallLaunched && !gameOver) {
            float alpha = (sinf(GetTime() * 4.0f) + 1.0f) * 0.3f + 0.4f;
            DrawText("SPACE: Launch | S: Split balls!", 
                     screenWidth/2 - 150, 400, 20, Fade(YELLOW, alpha));
        }
        
        DrawText("S:Split | L:Async | P:Pause | R:Reset", 
                 20, screenHeight - 25, 14, Fade(WHITE, 0.5f));
        
        // 效果提示
        if (gameSpeed > 1.0f)
            DrawText(">>> SPEED BOOST <<<", screenWidth/2 - 80, screenHeight - 50, 16, GREEN);
        else if (gameSpeed < 1.0f)
            DrawText("<<< SLOW MOTION >>>", screenWidth/2 - 80, screenHeight - 50, 16, BLUE);
        
        if (paddle.GetRect().width > 130)
            DrawText("WIDE PADDLE!", screenWidth/2 - 50, screenHeight - 70, 16, YELLOW);
        
        // 暂停
        if (paused && !gameOver) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
            DrawText("PAUSED", screenWidth/2 - 80, screenHeight/2 - 30, 48, YELLOW);
            DrawText("Press P to Continue", screenWidth/2 - 90, screenHeight/2 + 30, 20, WHITE);
        }
        
        // 游戏结束
        if (gameOver) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.8f));
            if (victory) {
                DrawText("VICTORY!", screenWidth/2 - 80, screenHeight/2 - 30, 40, GREEN);
            } else {
                DrawText("GAME OVER", screenWidth/2 - 100, screenHeight/2 - 30, 40, RED);
            }
            DrawText(TextFormat("Score: %d", score), screenWidth/2 - 50, screenHeight/2 + 10, 24, YELLOW);
            DrawText("Press R to Restart", screenWidth/2 - 80, screenHeight/2 + 50, 20, WHITE);
        }
        
        // 加载界面
        if (asyncLoader.IsLoading() || (loadCompleted && completionTimer < 2.0f)) {
            asyncLoader.DrawLoadingScreen(screenWidth, screenHeight);
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}
