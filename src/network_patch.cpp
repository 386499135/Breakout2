// network_patch.cpp - 网络功能集成补丁
// 使用方法：将此代码整合到 main.cpp 中，或单独包含

#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "PowerUp.h"
#include "NetworkManager.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <algorithm>
#include <random>
#include <cmath>

// ==================== 网络游戏状态结构 ====================
struct NetworkGameState {
    std::vector<Ball>* balls;
    Paddle* paddle;
    int* lives;
    int* score;
    float* gameSpeed;
    std::vector<Brick>* bricks;
    std::vector<std::unique_ptr<PowerUp>>* powerUps;
    ParticleSystem* particleSystem;
    int* winCount;
    bool* gameOver;
    bool* victory;
    bool* paused;
    float* gameTime;
};

// ==================== 全局网络变量 ====================
enum class NetworkGameMode {
    SINGLE_PLAYER,
    MULTIPLAYER_HOST,
    MULTIPLAYER_CLIENT,
    MENU
};

// 网络菜单状态
struct NetworkMenuState {
    NetworkGameMode selectedMode = NetworkGameMode::MENU;
    char serverAddress[32] = "127.0.0.1";
    bool isTyping = false;
    float cursorBlinkTimer = 0.0f;
    bool showCursor = true;
    char statusMessage[128] = "";
    float statusMessageTimer = 0.0f;
    bool connectionInProgress = false;
};

// ==================== 网络回调函数设置 ====================
void SetupNetworkCallbacks(NetworkManager& net, NetworkGameState& state, 
                          std::vector<Paddle>& remotePaddles) {
    
    // 远程玩家挡板更新回调
    net.SetOnPaddleUpdate([&remotePaddles](uint8_t playerId, const PaddleState& paddleState) {
        // 查找并更新对应的远程玩家挡板
        for (auto& paddle : remotePaddles) {
            if (paddle.GetPlayerId() == playerId) {
                paddle.SetPosition({paddleState.position.x, paddleState.position.y});
                
                // 更新挡板宽度（如果有道具效果）
                Rectangle rect = paddle.GetRect();
                if (rect.width != paddleState.width) {
                    rect.width = paddleState.width;
                    paddle.SetRect(rect);
                }
                break;
            }
        }
    });
    
    // 球状态更新回调（仅客户端使用）
    net.SetOnBallUpdate([state](const std::vector<BallState>& ballStates) {
        if (!state.balls) return;
        
        // 同步球的数量
        while (state.balls->size() < ballStates.size()) {
            state.balls->emplace_back(
                Vector2{400.0f, 530.0f}, 
                Vector2{0.0f, 0.0f}, 
                10.0f
            );
        }
        
        // 更新每个球的状态
        for (size_t i = 0; i < ballStates.size() && i < state.balls->size(); i++) {
            (*state.balls)[i].SetPosition({
                ballStates[i].position.x, 
                ballStates[i].position.y
            });
            (*state.balls)[i].SetSpeed({
                ballStates[i].velocity.x, 
                ballStates[i].velocity.y
            });
            (*state.balls)[i].SetLaunched(ballStates[i].launched);
        }
    });
    
    // 砖块销毁回调
    net.SetOnBrickDestroyed([state](uint8_t playerId, uint8_t brickIndex) {
        if (state.bricks && brickIndex < state.bricks->size()) {
            if ((*state.bricks)[brickIndex].IsActive()) {
                (*state.bricks)[brickIndex].SetActive(false);
                (*state.winCount)--;
                
                // 增加分数
                if (state.score) {
                    *state.score += 10;
                }
                
                // 生成粒子效果
                if (state.particleSystem) {
                    Vector2 brickCenter = {
                        (*state.bricks)[brickIndex].GetRect().x + 
                        (*state.bricks)[brickIndex].GetRect().width / 2,
                        (*state.bricks)[brickIndex].GetRect().y + 
                        (*state.bricks)[brickIndex].GetRect().height / 2
                    };
                    state.particleSystem->EmitBrickBreak(
                        brickCenter, 
                        (*state.bricks)[brickIndex].GetColor(), 
                        12
                    );
                }
            }
        }
    });
    
    // 道具生成回调
    net.SetOnPowerUpSpawn([state](uint8_t playerId, uint8_t powerUpType, Vector2 position) {
        if (state.powerUps) {
            PowerUpFactory factory;
            auto powerUp = factory.CreatePowerUp(
                powerUpType == 0 ? "speed_boost" :
                powerUpType == 1 ? "slow_motion" :
                powerUpType == 2 ? "multi_ball" :
                powerUpType == 3 ? "expand_paddle" : "extra_life",
                position
            );
            if (powerUp) {
                state.powerUps->push_back(std::move(powerUp));
            }
        }
    });
    
    // 玩家连接回调
    net.SetOnPlayerConnected([&remotePaddles](uint8_t playerId) {
        // 检查是否已存在
        for (const auto& paddle : remotePaddles) {
            if (paddle.GetPlayerId() == playerId) return;
        }
        
        // 创建新玩家的挡板
        Paddle newPaddle(340.0f, 550.0f, 120.0f, 15.0f);
        newPaddle.SetPlayerId(playerId);
        
        // 根据玩家ID设置不同颜色
        Color playerColor;
        switch(playerId) {
            case 0: playerColor = BLUE; break;
            case 1: playerColor = GREEN; break;
            case 2: playerColor = YELLOW; break;
            case 3: playerColor = PURPLE; break;
            default: playerColor = ColorFromHSV(playerId * 60, 1.0f, 1.0f);
        }
        newPaddle.SetColor(playerColor);
        
        remotePaddles.push_back(newPaddle);
    });
    
    // 玩家断线回调
    net.SetOnPlayerDisconnected([&remotePaddles](uint8_t playerId) {
        remotePaddles.erase(
            std::remove_if(remotePaddles.begin(), remotePaddles.end(),
                [playerId](const Paddle& p) { 
                    return p.GetPlayerId() == playerId; 
                }),
            remotePaddles.end()
        );
    });
}

// ==================== 网络菜单绘制 ====================
void DrawNetworkMenu(NetworkMenuState& menuState, NetworkManager& net, 
                     bool& menuActive, bool& networkGameActive) {
    
    const int screenWidth = 800;
    const int screenHeight = 600;
    
    // 背景
    DrawRectangle(0, 0, screenWidth, screenHeight, Color{20, 20, 30, 255});
    
    // 标题
    DrawText("BREAKOUT MULTIPLAYER", 200, 80, 36, GOLD);
    DrawText("Select Game Mode", 270, 130, 28, WHITE);
    
    // 单机模式按钮
    Rectangle singlePlayerBtn = {250, 190, 300, 55};
    Color singleColor = CheckCollisionPointRec(GetMousePosition(), singlePlayerBtn) ? 
                        Color{50, 50, 200, 255} : BLUE;
    DrawRectangleRec(singlePlayerBtn, singleColor);
    DrawRectangleLinesEx(singlePlayerBtn, 2, WHITE);
    DrawText("Single Player", 315, 205, 24, WHITE);
    
    // 主机模式按钮
    Rectangle hostBtn = {250, 265, 300, 55};
    Color hostColor = CheckCollisionPointRec(GetMousePosition(), hostBtn) ? 
                      Color{50, 200, 50, 255} : GREEN;
    DrawRectangleRec(hostBtn, hostColor);
    DrawRectangleLinesEx(hostBtn, 2, WHITE);
    DrawText("Host Game", 335, 280, 24, WHITE);
    
    // 客户端模式按钮
    Rectangle clientBtn = {250, 340, 300, 55};
    Color clientColor = CheckCollisionPointRec(GetMousePosition(), clientBtn) ? 
                        Color{255, 140, 0, 255} : ORANGE;
    DrawRectangleRec(clientBtn, clientColor);
    DrawRectangleLinesEx(clientBtn, 2, WHITE);
    DrawText("Join Game", 340, 355, 24, WHITE);
    
    // 服务器地址输入（仅客户端模式）
    if (menuState.selectedMode == NetworkGameMode::MULTIPLAYER_CLIENT) {
        DrawText("Server Address:", 220, 430, 20, LIGHTGRAY);
        
        Rectangle inputBox = {320, 425, 260, 35};
        DrawRectangleRec(inputBox, Color{40, 40, 50, 255});
        DrawRectangleLinesEx(inputBox, 1, menuState.isTyping ? YELLOW : GRAY);
        
        // 显示输入的地址
        DrawText(menuState.serverAddress, 330, 433, 20, WHITE);
        
        // 光标闪烁
        if (menuState.isTyping) {
            menuState.cursorBlinkTimer += GetFrameTime();
            if (menuState.cursorBlinkTimer > 0.5f) {
                menuState.showCursor = !menuState.showCursor;
                menuState.cursorBlinkTimer = 0.0f;
            }
            
            if (menuState.showCursor) {
                int textWidth = MeasureText(menuState.serverAddress, 20);
                DrawText("_", 330 + textWidth, 433, 20, YELLOW);
            }
        }
        
        // 连接按钮
        Rectangle connectBtn = {320, 475, 160, 40};
        Color connectColor = CheckCollisionPointRec(GetMousePosition(), connectBtn) ? 
                            Color{100, 100, 255, 255} : DARKBLUE;
        DrawRectangleRec(connectBtn, connectColor);
        DrawRectangleLinesEx(connectBtn, 1, WHITE);
        DrawText("Connect", 355, 485, 20, WHITE);
        
        // 处理连接
        if (!menuState.connectionInProgress && 
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(GetMousePosition(), connectBtn)) {
            
            menuState.connectionInProgress = true;
            strcpy(menuState.statusMessage, "Connecting to server...");
            
            if (net.ConnectToHost(menuState.serverAddress)) {
                menuActive = false;
                networkGameActive = true;
                strcpy(menuState.statusMessage, "Connected!");
            } else {
                strcpy(menuState.statusMessage, "Connection failed!");
                menuState.connectionInProgress = false;
                menuState.statusMessageTimer = 3.0f;
            }
        }
    }
    
    // 处理按钮点击
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        
        if (CheckCollisionPointRec(mousePos, singlePlayerBtn)) {
            menuState.selectedMode = NetworkGameMode::SINGLE_PLAYER;
            menuActive = false;
            networkGameActive = false;
        } 
        else if (CheckCollisionPointRec(mousePos, hostBtn)) {
            menuState.selectedMode = NetworkGameMode::MULTIPLAYER_HOST;
            strcpy(menuState.statusMessage, "Starting host...");
            
            if (net.StartHost()) {
                menuActive = false;
                networkGameActive = true;
                strcpy(menuState.statusMessage, "Host started! Waiting for players...");
            } else {
                strcpy(menuState.statusMessage, "Failed to start host!");
                menuState.statusMessageTimer = 3.0f;
            }
        } 
        else if (CheckCollisionPointRec(mousePos, clientBtn)) {
            menuState.selectedMode = NetworkGameMode::MULTIPLAYER_CLIENT;
        }
        
        // 点击输入框
        if (menuState.selectedMode == NetworkGameMode::MULTIPLAYER_CLIENT) {
            Rectangle inputBox = {320, 425, 260, 35};
            menuState.isTyping = CheckCollisionPointRec(mousePos, inputBox);
        }
    }
    
    // 处理键盘输入
    if (menuState.isTyping) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 125) {
                int len = strlen(menuState.serverAddress);
                if (len < 31) {
                    menuState.serverAddress[len] = (char)key;
                    menuState.serverAddress[len + 1] = '\0';
                }
            }
            key = GetCharPressed();
        }
        
        if (IsKeyPressed(KEY_BACKSPACE)) {
            int len = strlen(menuState.serverAddress);
            if (len > 0) {
                menuState.serverAddress[len - 1] = '\0';
            }
        }
        
        if (IsKeyPressed(KEY_ENTER)) {
            menuState.isTyping = false;
        }
    }
    
    // 显示状态消息
    if (menuState.statusMessageTimer > 0) {
        DrawText(menuState.statusMessage, 250, 550, 20, 
                strstr(menuState.statusMessage, "fail") ? RED : GREEN);
        menuState.statusMessageTimer -= GetFrameTime();
    }
    
    // 显示提示
    DrawText("Press ESC to exit", 320, 570, 16, GRAY);
}

// ==================== 网络状态显示 ====================
void DrawNetworkStatus(NetworkManager& net, NetworkGameMode mode, 
                       int remotePlayerCount) {
    
    const int screenHeight = 600;
    
    if (mode == NetworkGameMode::SINGLE_PLAYER) {
        DrawText("Mode: Single Player", 10, screenHeight - 30, 20, GRAY);
        return;
    }
    
    // 网络模式标识
    const char* modeText = (mode == NetworkGameMode::MULTIPLAYER_HOST) ? 
                           "Host" : "Client";
    Color modeColor = (mode == NetworkGameMode::MULTIPLAYER_HOST) ? 
                      GREEN : ORANGE;
    
    DrawText(TextFormat("Mode: %s", modeText), 10, screenHeight - 30, 20, modeColor);
    
    // 延迟显示
    float latency = net.GetAverageLatency();
    Color latencyColor = latency < 50 ? GREEN : (latency < 100 ? YELLOW : RED);
    DrawText(TextFormat("Ping: %.0fms", latency), 150, screenHeight - 30, 20, latencyColor);
    
    // 玩家数量
    DrawText(TextFormat("Players: %d/4", remotePlayerCount + 1), 
             300, screenHeight - 30, 20, WHITE);
    
    // 丢包模拟状态
    static bool lossSimEnabled = false;
    if (IsKeyPressed(KEY_F4)) {
        lossSimEnabled = !lossSimEnabled;
        net.EnablePacketLossSimulation(lossSimEnabled, 0.15f);
    }
    
    if (lossSimEnabled) {
        DrawText("Packet Loss Sim: ON", 480, screenHeight - 30, 20, RED);
    }
    
    // 显示提示
    DrawText("F3:Stats F4:LossSim", 10, screenHeight - 55, 16, GRAY);
}

// ==================== 网络游戏状态同步 ====================
void SyncNetworkGameState(NetworkManager& net, NetworkGameMode mode, 
                          NetworkGameState& state, Paddle& localPaddle) {
    
    if (mode == NetworkGameMode::SINGLE_PLAYER) return;
    
    // 发送本地挡板状态
    PaddleState localPaddleState;
    localPaddleState.playerId = net.GetLocalPlayerId();
    localPaddleState.position.x = localPaddle.GetRect().x;
    localPaddleState.position.y = localPaddle.GetRect().y;
    localPaddleState.width = localPaddle.GetRect().width;
    net.SendPaddleState(localPaddleState);
    
    // 主机发送游戏状态
    if (mode == NetworkGameMode::MULTIPLAYER_HOST) {
        // 收集球的状态
        std::vector<BallState> ballStates;
        for (const auto& ball : *state.balls) {
            BallState ballState;
            ballState.ballId = ballStates.size();
            ballState.position.x = ball.GetPosition().x;
            ballState.position.y = ball.GetPosition().y;
            ballState.velocity.x = ball.GetSpeed().x;
            ballState.velocity.y = ball.GetSpeed().y;
            ballState.radius = ball.GetRadius();
            ballState.launched = ball.IsLaunched();
            ballStates.push_back(ballState);
        }
        
        net.SendBallState(ballStates);
    }
    
    // 主机处理远程玩家的输入（用于发射球等）
    if (mode == NetworkGameMode::MULTIPLAYER_HOST) {
        // 这里可以处理远程玩家的特殊操作
        // 例如：当远程玩家按下空格时发射球
    }
}

// ==================== 网络事件处理 ====================
void HandleNetworkEvents(NetworkManager& net, NetworkGameMode mode,
                         NetworkGameState& state, Paddle& localPaddle,
                         PowerUpFactory& powerUpFactory,
                         std::random_device& rd, std::mt19937& gen,
                         std::uniform_real_distribution<float>& dropDist) {
    
    if (mode == NetworkGameMode::SINGLE_PLAYER) return;
    
    // 检查砖块碰撞并通知网络（仅主机）
    if (mode == NetworkGameMode::MULTIPLAYER_HOST) {
        for (size_t i = 0; i < state.bricks->size(); i++) {
            if (!(*state.bricks)[i].IsActive()) continue;
            
            for (auto& ball : *state.balls) {
                if (!ball.IsLaunched()) continue;
                
                if (ball.CheckBrickCollision((*state.bricks)[i].GetRect())) {
                    // 通知所有客户端砖块被销毁
                    net.SendBrickDestroyed(i, {
                        (*state.bricks)[i].GetRect().x + (*state.bricks)[i].GetRect().width / 2,
                        (*state.bricks)[i].GetRect().y + (*state.bricks)[i].GetRect().height / 2
                    });
                    
                    (*state.bricks)[i].SetActive(false);
                    *state.score += CalculateScore(10, *state.gameTime);
                    (*state.winCount)--;
                    
                    // 生成粒子
                    Vector2 brickCenter = {
                        (*state.bricks)[i].GetRect().x + (*state.bricks)[i].GetRect().width / 2,
                        (*state.bricks)[i].GetRect().y + (*state.bricks)[i].GetRect().height / 2
                    };
                    state.particleSystem->EmitBrickBreak(
                        brickCenter, 
                        (*state.bricks)[i].GetColor(), 
                        12
                    );
                    
                    // 道具掉落
                    if (dropDist(gen) < powerUpFactory.GetDropChance()) {
                        auto powerUp = powerUpFactory.CreateRandomPowerUp(brickCenter);
                        if (powerUp) {
                            // 通知客户端道具生成
                            uint8_t powerUpType = 0; // 简化处理
                            net.SendPowerUpSpawn(powerUpType, brickCenter);
                            state.powerUps->push_back(std::move(powerUp));
                        }
                    }
                    
                    break;
                }
            }
        }
    }
}

// ==================== 远程玩家挡板绘制 ====================
void DrawRemotePaddles(const std::vector<Paddle>& remotePaddles) {
    for (const auto& paddle : remotePaddles) {
        paddle.Draw();
        
        // 显示玩家ID
        DrawText(TextFormat("P%d", paddle.GetPlayerId()), 
                (int)paddle.GetRect().x + 5, 
                (int)paddle.GetRect().y - 20, 
                16, WHITE);
    }
}

// ==================== 主游戏集成示例 ====================
/*
// 在主游戏循环中使用这些函数的示例：

int main() {
    // ... 初始化代码 ...
    
    // 网络初始化
    NetworkManager networkManager;
    networkManager.Initialize();
    
    NetworkMenuState menuState;
    bool networkMenuActive = true;
    bool networkGameActive = false;
    NetworkGameMode gameMode = NetworkGameMode::MENU;
    
    std::vector<Paddle> remotePaddles;
    NetworkGameState networkState;
    
    // 主循环
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        // 网络菜单
        if (networkMenuActive) {
            BeginDrawing();
            DrawNetworkMenu(menuState, networkManager, networkMenuActive, networkGameActive);
            EndDrawing();
            
            if (!networkMenuActive) {
                gameMode = menuState.selectedMode;
                if (gameMode != NetworkGameMode::SINGLE_PLAYER) {
                    SetupNetworkCallbacks(networkManager, networkState, remotePaddles);
                }
            }
            continue;
        }
        
        // 网络更新
        if (networkGameActive) {
            networkManager.Update(deltaTime);
            
            // 按F3显示网络统计
            if (IsKeyPressed(KEY_F3)) {
                networkManager.PrintNetworkStats();
            }
        }
        
        // ... 游戏逻辑 ...
        
        // 同步网络状态
        if (networkGameActive) {
            SyncNetworkGameState(networkManager, gameMode, networkState, paddle);
        }
        
        // 渲染
        BeginDrawing();
        ClearBackground(Color{30, 30, 40, 255});
        
        // 绘制游戏对象
        for (auto& brick : bricks) brick.Draw();
        paddle.Draw();
        DrawRemotePaddles(remotePaddles);
        for (auto& ball : balls) ball.Draw();
        
        // 绘制网络状态
        if (networkGameActive) {
            DrawNetworkStatus(networkManager, gameMode, remotePaddles.size());
        }
        
        EndDrawing();
    }
    
    networkManager.Shutdown();
    CloseWindow();
    return 0;
}
*/
