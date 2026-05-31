#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "PowerUp.h"
#include "ParticleSystem.h"
#include "SaveManager.h"
#include "AsyncLoader.h"
#include "Leaderboard.h"
#include "NetworkManager.h"
#include "GameConstants.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <random>
#include <ctime>
#include <iostream>

enum class GameMode { MENU, PLAYING, PAUSED, GAME_OVER, LEVEL_SELECT, NETWORK_MENU };
enum class NetMode { SINGLE, HOST, CLIENT };

void DrawRoundedRect(Rectangle rect, float roundness, Color color) {
    DrawRectangleRounded(rect, roundness, 8, color);
}

void DrawGlowText(const char* text, int x, int y, int fontSize, Color color) {
    for (int i = 1; i <= 2; i++) {
        DrawText(text, x - i, y, fontSize, Fade(color, 0.15f));
        DrawText(text, x + i, y, fontSize, Fade(color, 0.15f));
        DrawText(text, x, y - i, fontSize, Fade(color, 0.15f));
        DrawText(text, x, y + i, fontSize, Fade(color, 0.15f));
    }
    DrawText(text, x, y, fontSize, color);
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "BREAKOUT - Enhanced Edition");
    SetTargetFPS(60);
    srand(time(nullptr));
    
    SaveManager saveManager;
    Leaderboard leaderboard("scores.txt");
    AsyncLoader asyncLoader;
    NetworkManager networkManager;
    
    GameMode gameMode = GameMode::MENU;
    NetMode netMode = NetMode::SINGLE;
    int menuSelection = 0;
    int selectedLevel = 1;
    
    std::vector<Ball> balls;
    Paddle paddle(340, 550, 120, 15);
    std::vector<Brick> bricks;
    std::vector<std::unique_ptr<PowerUp>> powerUps;
    ParticleSystem particleSystem;
    PowerUpFactory powerUpFactory;
    
    int score = 0, lives = 5, currentLevel = 1, winCount = 0;
    float gameTime = 0.0f, gameSpeed = 1.0f, splitCooldown = 0.0f;
    bool anyBallLaunched = false, gameOverFlag = false, victoryFlag = false;
    bool showSaveMessage = false, saveMessageTimer = 0.0f;
    bool levelTransition = false;  // 防止重复切换关卡
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dropDist(0.0f, 1.0f);
    std::vector<std::pair<std::unique_ptr<PowerUpEffect>, float>> activeEffects;
    
    auto LoadLevel = [&](int level, const std::vector<bool>* savedStates = nullptr) {
        bricks = saveManager.LoadLevel(level, savedStates);
        winCount = 0;
        for (const auto& brick : bricks) if (brick.IsActive()) winCount++;
        currentLevel = level;
        levelTransition = false;
        std::cout << "加载关卡 " << level << std::endl;
    };
    
    auto ResetGame = [&]() {
        balls.clear();
        balls.emplace_back(Vector2{400, 530}, Vector2{0, 0}, 10);
        paddle = Paddle(340, 550, 120, 15);
        powerUps.clear();
        activeEffects.clear();
        particleSystem.Clear();
        score = 0; lives = 5; gameSpeed = 1.0f; splitCooldown = 0.0f;
        anyBallLaunched = false; gameOverFlag = false; victoryFlag = false;
        gameTime = 0.0f; levelTransition = false;
    };
    
    auto NewGame = [&]() { ResetGame(); LoadLevel(1); gameMode = GameMode::PLAYING; };
    
    auto ContinueGame = [&]() {
        SaveData data;
        if (saveManager.LoadGame(data)) {
            ResetGame();
            score = data.score; lives = data.lives; gameTime = data.gameTime; gameSpeed = data.gameSpeed;
            LoadLevel(data.currentLevel, &data.brickStates);
            gameMode = GameMode::PLAYING;
        } else NewGame();
    };
    
    auto SaveCurrentGame = [&]() {
        if (saveManager.SaveGame(score, lives, currentLevel, gameTime, gameSpeed, bricks)) {
            showSaveMessage = true; saveMessageTimer = 2.0f;
        }
    };
    
    auto NextLevel = [&]() {
        if (!levelTransition && currentLevel < saveManager.GetLevelCount()) {
            levelTransition = true;
            LoadLevel(currentLevel + 1);
            ResetGame();
        } else if (!levelTransition && currentLevel >= saveManager.GetLevelCount()) {
            levelTransition = true;
            gameOverFlag = true; victoryFlag = true;
            gameMode = GameMode::GAME_OVER;
            leaderboard.AddScore("Player", score);
        }
    };
    
    LoadLevel(1);
    
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        if (saveMessageTimer > 0) { saveMessageTimer -= deltaTime; if (saveMessageTimer <= 0) showSaveMessage = false; }
        if (splitCooldown > 0) splitCooldown -= deltaTime;
        asyncLoader.Update(deltaTime);
        networkManager.Update();
        
        // 菜单
        if (gameMode == GameMode::MENU) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) menuSelection = (menuSelection - 1 + 5) % 5;
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menuSelection = (menuSelection + 1) % 5;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                switch (menuSelection) {
                    case 0: NewGame(); break;
                    case 1: ContinueGame(); break;
                    case 2: gameMode = GameMode::LEVEL_SELECT; break;
                    case 3: gameMode = GameMode::NETWORK_MENU; break;
                    case 4: CloseWindow(); return 0;
                }
            }
        }
        // 网络菜单
        else if (gameMode == GameMode::NETWORK_MENU) {
            if (IsKeyPressed(KEY_ESCAPE)) gameMode = GameMode::MENU;
            if (IsKeyPressed(KEY_ONE)) { netMode = NetMode::SINGLE; NewGame(); }
            if (IsKeyPressed(KEY_TWO)) { netMode = NetMode::HOST; networkManager.StartHost(); NewGame(); }
            if (IsKeyPressed(KEY_THREE)) { netMode = NetMode::CLIENT; networkManager.ConnectToHost("127.0.0.1"); NewGame(); }
        }
        // 关卡选择
        else if (gameMode == GameMode::LEVEL_SELECT) {
            if (IsKeyPressed(KEY_ESCAPE)) gameMode = GameMode::MENU;
            for (int i = 1; i <= saveManager.GetLevelCount(); i++) {
                if (IsKeyPressed(KEY_ONE + i - 1)) {
                    ResetGame(); LoadLevel(i); gameMode = GameMode::PLAYING;
                }
            }
        }
        // 游戏进行
        else if (gameMode == GameMode::PLAYING && !gameOverFlag) {
            if (IsKeyPressed(KEY_P)) gameMode = GameMode::PAUSED;
            if (IsKeyPressed(KEY_F5)) SaveCurrentGame();
            if (IsKeyPressed(KEY_R)) { NewGame(); continue; }
            
            if (IsKeyPressed(KEY_S) && splitCooldown <= 0 && !balls.empty()) {
                std::vector<Ball> newBalls;
                for (auto& ball : balls) {
                    if (ball.IsLaunched()) {
                        auto splits = ball.Split(2);
                        for (auto& b : splits) newBalls.push_back(b);
                    }
                }
                for (auto& b : newBalls) balls.push_back(std::move(b));
                splitCooldown = 2.0f;
            }
            
            float currentSpeed = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) ? PADDLE_SPEED_BOOST : PADDLE_SPEED_NORMAL;
            currentSpeed *= gameSpeed;
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) paddle.MoveLeft(currentSpeed);
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) paddle.MoveRight(currentSpeed);
            
            if (netMode != NetMode::SINGLE && networkManager.IsConnected()) {
                networkManager.SendPaddlePosition(paddle.GetRect().x);
            }
            
            anyBallLaunched = false;
            for (auto& ball : balls) {
                if (!ball.IsLaunched()) ball.ResetToPaddle(paddle.GetRect().x + paddle.GetRect().width / 2, paddle.GetRect().y);
                else anyBallLaunched = true;
            }
            
            if (!anyBallLaunched && IsKeyPressed(KEY_SPACE)) {
                balls[0].Launch(paddle.GetRect().x + paddle.GetRect().width / 2);
            }
            
            for (auto& ball : balls) {
                if (!ball.IsLaunched()) continue;
                ball.ApplyGravity(); ball.Move(); ball.BounceEdge(); ball.BouncePaddle(paddle.GetRect());
                
                if (netMode != NetMode::SINGLE && networkManager.IsConnected()) {
                    Rectangle remoteRect = {networkManager.GetRemotePaddleX(), 550, 120, 15};
                    ball.BouncePaddle(remoteRect);
                }
                
                for (auto& brick : bricks) {
                    if (brick.IsActive() && ball.CheckBrickCollision(brick.GetRect())) {
                        if (brick.Hit()) {
                            score += 10; winCount--;
                            particleSystem.EmitBrickBreak({brick.GetRect().x + brick.GetRect().width/2, brick.GetRect().y + brick.GetRect().height/2}, brick.GetColor(), 15);
                            if (dropDist(gen) < powerUpFactory.GetDropChance()) {
                                auto powerUp = powerUpFactory.CreateRandomPowerUp({brick.GetRect().x + brick.GetRect().width/2, brick.GetRect().y + brick.GetRect().height/2});
                                if (powerUp) powerUps.push_back(std::move(powerUp));
                            }
                        }
                        break;
                    }
                }
            }
            
            balls.erase(std::remove_if(balls.begin(), balls.end(), [](Ball& ball) { return ball.GetPosition().y > SCREEN_HEIGHT + 200; }), balls.end());
            
            if (balls.empty()) {
                lives--;
                if (lives <= 0) {
                    gameOverFlag = true; victoryFlag = false;
                    gameMode = GameMode::GAME_OVER;
                    leaderboard.AddScore("Player", score);
                } else {
                    balls.emplace_back(Vector2{400, 530}, Vector2{0, 0}, 10);
                    anyBallLaunched = false;
                }
            }
            
            // 通关检查 - 使用 levelTransition 防止重复
            if (winCount <= 0 && !gameOverFlag && !levelTransition) {
                NextLevel();
            }
            
            for (auto it = powerUps.begin(); it != powerUps.end();) {
                (*it)->Update(deltaTime);
                if ((*it)->CheckCollision(paddle.GetRect())) {
                    GameState state(&balls, &paddle, &lives, &score, &gameSpeed);
                    (*it)->Apply(&state);
                    if ((*it)->GetEffect()->IsTimed()) {
                        activeEffects.emplace_back(std::unique_ptr<PowerUpEffect>((*it)->GetEffect()), (*it)->GetEffect()->GetDuration());
                    }
                    it = powerUps.erase(it);
                } else if (!(*it)->IsActive()) { it = powerUps.erase(it); }
                else { ++it; }
            }
            
            for (auto it = activeEffects.begin(); it != activeEffects.end();) {
                it->second -= deltaTime;
                if (it->second <= 0) {
                    GameState state(&balls, &paddle, &lives, &score, &gameSpeed);
                    it->first->Remove(&state);
                    it = activeEffects.erase(it);
                } else { ++it; }
            }
            
            particleSystem.Update(deltaTime);
            gameTime += deltaTime;
        }
        // 暂停
        else if (gameMode == GameMode::PAUSED) {
            if (IsKeyPressed(KEY_P)) gameMode = GameMode::PLAYING;
            if (IsKeyPressed(KEY_ESCAPE)) gameMode = GameMode::MENU;
        }
        // 游戏结束
        else if (gameMode == GameMode::GAME_OVER) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) NewGame();
            if (IsKeyPressed(KEY_ESCAPE)) gameMode = GameMode::MENU;
        }
        
        // 渲染
        BeginDrawing();
        ClearBackground(COLOR_BG);
        
        if (gameMode == GameMode::MENU) {
            DrawGlowText("BREAKOUT", SCREEN_WIDTH/2 - 100, 60, 48, COLOR_GOLD);
            DrawText("Enhanced Edition", SCREEN_WIDTH/2 - 80, 115, 20, Fade(WHITE, 0.6f));
            const char* options[] = {"New Game", "Continue", "Level Select", "Multiplayer", "Exit"};
            for (int i = 0; i < 5; i++) {
                Color color = (menuSelection == i) ? COLOR_GOLD : Fade(WHITE, 0.7f);
                DrawText(TextFormat("%s %s", (menuSelection == i) ? ">" : " ", options[i]), SCREEN_WIDTH/2 - 60, 180 + i * 45, 24, color);
            }
            if (saveManager.HasSaveGame()) DrawText(TextFormat("Save: %s", saveManager.GetSaveTimeString().c_str()), SCREEN_WIDTH/2 - 150, 420, 14, Fade(GREEN, 0.7f));
            DrawText("UP/DOWN: Navigate | ENTER: Select", 20, SCREEN_HEIGHT - 30, 14, Fade(WHITE, 0.4f));
        }
        else if (gameMode == GameMode::NETWORK_MENU) {
            DrawText("MULTIPLAYER", SCREEN_WIDTH/2 - 90, 60, 32, COLOR_GOLD);
            DrawText("1 - Single Player", SCREEN_WIDTH/2 - 100, 150, 24, WHITE);
            DrawText("2 - Host Game", SCREEN_WIDTH/2 - 100, 210, 24, WHITE);
            DrawText("3 - Join Game", SCREEN_WIDTH/2 - 100, 270, 24, WHITE);
            DrawText("Press ESC to return", SCREEN_WIDTH/2 - 90, 400, 16, Fade(WHITE, 0.5f));
        }
        else if (gameMode == GameMode::LEVEL_SELECT) {
            DrawText("SELECT LEVEL", SCREEN_WIDTH/2 - 80, 60, 32, COLOR_GOLD);
            DrawText("Press ESC to return", SCREEN_WIDTH/2 - 90, 100, 16, Fade(WHITE, 0.5f));
            for (int i = 1; i <= saveManager.GetLevelCount(); i++) {
                Color color = (selectedLevel == i) ? COLOR_ACCENT : Fade(WHITE, 0.6f);
                DrawText(TextFormat("%d. %s", i, saveManager.GetLevelName(i).c_str()), SCREEN_WIDTH/2 - 80, 150 + i * 50, 28, color);
            }
            DrawText("Press number key to select", SCREEN_WIDTH/2 - 110, SCREEN_HEIGHT - 50, 16, Fade(WHITE, 0.4f));
        }
        else if (gameMode == GameMode::PLAYING || gameMode == GameMode::PAUSED) {
            DrawRectangle(0, 0, 5, SCREEN_HEIGHT, COLOR_BORDER);
            DrawRectangle(SCREEN_WIDTH - 5, 0, 5, SCREEN_HEIGHT, COLOR_BORDER);
            DrawRectangle(0, 0, SCREEN_WIDTH, 5, COLOR_BORDER);
            
            for (auto& brick : bricks) brick.Draw();
            paddle.Draw();
            
            if (netMode != NetMode::SINGLE && networkManager.IsConnected()) {
                DrawRectangleRounded({networkManager.GetRemotePaddleX(), 550, 120, 15}, 0.3f, 8, Fade(GREEN, 0.5f));
                DrawRectangleRoundedLines({networkManager.GetRemotePaddleX(), 550, 120, 15}, 0.3f, 8, 2, GREEN);
            }
            
            for (auto& ball : balls) ball.Draw();
            for (auto& powerUp : powerUps) powerUp->Draw();
            particleSystem.Draw();
            
            DrawRoundedRect({10, 10, 120, 40}, 0.2f, COLOR_PANEL);
            DrawText(TextFormat("Score: %d", score), 20, 18, 20, WHITE);
            DrawRoundedRect({SCREEN_WIDTH - 130, 10, 120, 40}, 0.2f, COLOR_PANEL);
            DrawText(TextFormat("FPS: %d", GetFPS()), SCREEN_WIDTH - 120, 18, 20, LIME);
            DrawRoundedRect({10, 55, 100, 40}, 0.2f, COLOR_PANEL);
            Color lifeColor = lives > 3 ? COLOR_SUCCESS : (lives > 1 ? COLOR_WARNING : COLOR_DANGER);
            DrawText(TextFormat("Lives: %d", lives), 20, 63, 20, lifeColor);
            DrawRoundedRect({120, 55, 100, 40}, 0.2f, COLOR_PANEL);
            DrawText(TextFormat("Balls: %d", (int)balls.size()), 130, 63, 20, balls.size() > 3 ? PURPLE : WHITE);
            DrawRoundedRect({230, 55, 120, 40}, 0.2f, COLOR_PANEL);
            DrawText(TextFormat("Level: %d/3", currentLevel), 240, 63, 20, COLOR_ACCENT);
            
            if (netMode != NetMode::SINGLE) {
                DrawRoundedRect({SCREEN_WIDTH - 200, 55, 180, 40}, 0.2f, COLOR_PANEL);
                DrawText(TextFormat("Mode: %s", netMode == NetMode::HOST ? "HOST" : "CLIENT"), SCREEN_WIDTH - 190, 63, 18, networkManager.IsConnected() ? COLOR_SUCCESS : COLOR_WARNING);
            }
            
            if (!anyBallLaunched && !gameOverFlag) {
                float alpha = (sinf(GetTime() * 4.0f) + 1.0f) * 0.3f + 0.4f;
                DrawGlowText("PRESS SPACE TO LAUNCH", SCREEN_WIDTH/2 - 120, 350, 24, Fade(YELLOW, alpha));
            }
            
            DrawText("S:Split | P:Pause | R:Reset | F5:Save", 20, SCREEN_HEIGHT - 25, 14, Fade(WHITE, 0.5f));
            
            if (gameSpeed > 1.0f) DrawGlowText(">>> SPEED BOOST <<<", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT - 70, 18, GREEN);
            else if (gameSpeed < 0.9f) DrawGlowText("<<< SLOW MOTION >>>", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT - 70, 18, SKYBLUE);
            if (paddle.GetRect().width > 130) DrawGlowText("WIDE PADDLE!", SCREEN_WIDTH/2 - 50, SCREEN_HEIGHT - 90, 18, YELLOW);
            if (showSaveMessage) DrawGlowText("GAME SAVED!", SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT/2 - 50, 28, COLOR_SUCCESS);
            
            if (gameMode == GameMode::PAUSED) {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));
                DrawGlowText("PAUSED", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 40, 48, YELLOW);
                DrawText("Press P to Resume", SCREEN_WIDTH/2 - 90, SCREEN_HEIGHT/2 + 20, 20, WHITE);
            }
        }
        else if (gameMode == GameMode::GAME_OVER) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.85f));
            if (victoryFlag) {
                DrawGlowText("VICTORY!", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 60, 48, COLOR_GOLD);
                DrawText("You cleared all levels!", SCREEN_WIDTH/2 - 110, SCREEN_HEIGHT/2 - 10, 20, WHITE);
            } else {
                DrawGlowText("GAME OVER", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 60, 48, COLOR_DANGER);
            }
            DrawText(TextFormat("Final Score: %d", score), SCREEN_WIDTH/2 - 70, SCREEN_HEIGHT/2 + 20, 28, YELLOW);
            DrawText("Press ENTER to Play Again", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 + 100, 20, COLOR_SUCCESS);
            DrawText("Press ESC to Menu", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 140, 16, Fade(WHITE, 0.6f));
        }
        
        if (asyncLoader.IsLoading()) asyncLoader.DrawLoadingScreen(SCREEN_WIDTH, SCREEN_HEIGHT);
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}
