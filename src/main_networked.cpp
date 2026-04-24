#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "PowerUp.h"
#include "Network.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <algorithm>
#include <random>
#include <cmath>

// 游戏模式
enum class GameMode {
    MENU,
    SINGLE_PLAYER,
    MULTIPLAYER_HOST,
    MULTIPLAYER_CLIENT
};

// 颜色常量
const Color COLOR_BG = {15, 20, 30, 255};
const Color COLOR_PANEL = {25, 30, 45, 200};
const Color COLOR_ACCENT = {80, 140, 255, 255};
const Color COLOR_SUCCESS = {80, 200, 120, 255};
const Color COLOR_WARNING = {255, 180, 50, 255};
const Color COLOR_DANGER = {255, 80, 80, 255};
const Color COLOR_GOLD = {255, 215, 0, 255};
const Color COLOR_SILVER = {192, 192, 192, 255};
const Color COLOR_BRONZE = {205, 127, 50, 255};

// 中文字体
static Font chineseFont;
static bool fontLoaded = false;

void InitChineseFont() {
    const char* text = "分数生命暂停继续重新开始游戏结束胜利排行榜第名按P暂停按R重新开始时间倍率落地惩罚恭喜进入空格发射道具速度提升减速多重球加长挡板额外生命主机客户端连接等待中控制说明主机单人客户端左移右移发射空格键暂停重新开始剩余";
    int codepointCount = 0;
    int* codepoints = LoadCodepoints(text, &codepointCount);
    
    const char* fontPaths[] = { 
        "../fonts/NotoSansSC.otf",
        "NotoSansSC.otf",
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    };
    
    fontLoaded = false;
    
    for (int i = 0; i < 4; i++) {
        if (FileExists(fontPaths[i])) {
            printf("Loading font: %s\n", fontPaths[i]);
            chineseFont = LoadFontEx(fontPaths[i], 48, codepoints, codepointCount);
            if (chineseFont.texture.id != 0) { 
                fontLoaded = true; 
                printf("Font loaded successfully!\n");
                break; 
            }
        }
    }
    
    if (!fontLoaded) {
        printf("Using default font\n");
        chineseFont = GetFontDefault();
    }
    
    UnloadCodepoints(codepoints);
}

void DrawChineseText(const char* text, int x, int y, int fontSize, Color color) { 
    if (fontLoaded && chineseFont.texture.id != 0) {
        Vector2 pos = { (float)x, (float)y }; 
        DrawTextEx(chineseFont, text, pos, (float)fontSize, 2, color); 
    } else {
        DrawText(text, x, y, fontSize, color);
    }
}

void DrawChineseTextCentered(const char* text, int y, int fontSize, Color color) {
    if (fontLoaded && chineseFont.texture.id != 0) {
        Vector2 size = MeasureTextEx(chineseFont, text, fontSize, 2);
        DrawChineseText(text, (GetScreenWidth() - (int)size.x) / 2, y, fontSize, color);
    } else {
        int textWidth = MeasureText(text, fontSize);
        DrawText(text, (GetScreenWidth() - textWidth) / 2, y, fontSize, color);
    }
}

int CalculateScore(int baseScore, float gameTime) {
    float multiplier = 5.0f - gameTime * 0.05f;
    if (multiplier < 1.0f) multiplier = 1.0f;
    return (int)(baseScore * multiplier);
}

// 绘制带圆角的矩形
void DrawRoundedRect(Rectangle rect, float roundness, Color color) {
    DrawRectangleRounded(rect, roundness, 8, color);
}

// 绘制按钮
bool DrawButton(Rectangle rect, const char* text, Color color, Color hoverColor) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);
    
    DrawRectangleRounded(rect, 0.3f, 8, hovered ? hoverColor : color);
    DrawRectangleRoundedLines(rect, 0.3f, 8, 2, Fade(WHITE, 0.3f));
    
    int fontSize = 22;
    int textWidth = MeasureText(text, fontSize);
    DrawText(text, rect.x + (rect.width - textWidth) / 2, 
             rect.y + (rect.height - fontSize) / 2, fontSize, WHITE);
    
    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// 绘制渐变背景
void DrawGradientBackground() {
    for (int i = 0; i < GetScreenHeight(); i++) {
        float t = (float)i / GetScreenHeight();
        Color c = {
            (unsigned char)(15 + 10 * t),
            (unsigned char)(20 + 15 * t),
            (unsigned char)(35 + 20 * t),
            255
        };
        DrawRectangle(0, i, GetScreenWidth(), 1, c);
    }
}

// 绘制发光文字
void DrawGlowText(const char* text, int x, int y, int fontSize, Color color) {
    for (int i = 1; i <= 3; i++) {
        DrawText(text, x - i, y, fontSize, Fade(color, 0.2f));
        DrawText(text, x + i, y, fontSize, Fade(color, 0.2f));
        DrawText(text, x, y - i, fontSize, Fade(color, 0.2f));
        DrawText(text, x, y + i, fontSize, Fade(color, 0.2f));
    }
    DrawText(text, x, y, fontSize, color);
}

// 绘制状态卡片
void DrawStatusCard(int x, int y, int width, int height, const char* title, 
                    const char* value, Color titleColor, Color valueColor) {
    DrawRectangleRounded({(float)x, (float)y, (float)width, (float)height}, 0.2f, 8, COLOR_PANEL);
    DrawRectangleRoundedLines({(float)x, (float)y, (float)width, (float)height}, 0.2f, 8, 1, Fade(WHITE, 0.2f));
    
    DrawText(title, x + 10, y + 8, 14, Fade(WHITE, 0.6f));
    DrawText(value, x + 10, y + 28, 28, valueColor);
}

// 绘制网络状态指示器
void DrawNetworkIndicator(int x, int y, bool connected, const char* role) {
    Color dotColor = connected ? COLOR_SUCCESS : COLOR_DANGER;
    DrawCircle(x, y, 6, dotColor);
    DrawCircle(x, y, 10, Fade(dotColor, 0.3f));
    
    DrawText(role, x + 18, y - 8, 16, connected ? COLOR_SUCCESS : Fade(WHITE, 0.5f));
    if (!connected && role[0] != '\0') {
        DrawText("Waiting...", x + 18, y + 10, 12, Fade(WHITE, 0.4f));
    }
}

// 绘制菜单
void DrawMenu(GameMode& mode, NetworkManager& net, bool& inGame, 
              char* serverIP, bool& typing) {
    
    DrawGradientBackground();
    
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();
    
    // 标题
    DrawGlowText("BREAKOUT", screenWidth/2 - 120, 60, 48, COLOR_GOLD);
    DrawText("MULTIPLAYER", screenWidth/2 - 80, 110, 20, Fade(WHITE, 0.6f));
    
    // 装饰线
    DrawLine(screenWidth/2 - 100, 140, screenWidth/2 + 100, 140, Fade(COLOR_ACCENT, 0.5f));
    
    DrawText("Select Game Mode", screenWidth/2 - 85, 165, 20, Fade(WHITE, 0.7f));
    
    // 按钮
    Rectangle singleBtn = {(float)(screenWidth/2 - 150), 200, 300, 55};
    Rectangle hostBtn = {(float)(screenWidth/2 - 150), 270, 300, 55};
    Rectangle clientBtn = {(float)(screenWidth/2 - 150), 340, 300, 55};
    
    if (DrawButton(singleBtn, "1P - Single Player", COLOR_ACCENT, Fade(COLOR_ACCENT, 0.8f))) {
        mode = GameMode::SINGLE_PLAYER;
        inGame = true;
    }
    
    if (DrawButton(hostBtn, "HOST - Create Game", COLOR_SUCCESS, Fade(COLOR_SUCCESS, 0.8f))) {
        if (net.StartHost(12345)) {
            mode = GameMode::MULTIPLAYER_HOST;
            inGame = true;
        }
    }
    
    if (DrawButton(clientBtn, "JOIN - Connect", COLOR_WARNING, Fade(COLOR_WARNING, 0.8f))) {
        mode = GameMode::MULTIPLAYER_CLIENT;
    }
    
    // IP输入框（客户端模式）
    if (mode == GameMode::MULTIPLAYER_CLIENT) {
        DrawText("Server Address", screenWidth/2 - 130, 420, 16, Fade(WHITE, 0.6f));
        
        Rectangle inputBox = {(float)(screenWidth/2 - 130), 440, 200, 40};
        DrawRectangleRounded(inputBox, 0.2f, 8, Color{30, 35, 50, 255});
        if (typing) {
            DrawRectangleRoundedLines(inputBox, 0.2f, 8, 2, COLOR_ACCENT);
        } else {
            DrawRectangleRoundedLines(inputBox, 0.2f, 8, 1, Fade(WHITE, 0.3f));
        }
        DrawText(serverIP, inputBox.x + 12, inputBox.y + 12, 18, WHITE);
        
        // 光标闪烁
        if (typing && ((int)(GetTime() * 2) % 2 == 0)) {
            int textWidth = MeasureText(serverIP, 18);
            DrawText("|", inputBox.x + 14 + textWidth, inputBox.y + 10, 20, COLOR_ACCENT);
        }
        
        Rectangle connectBtn = {(float)(screenWidth/2 + 80), 440, 50, 40};
        if (DrawButton(connectBtn, ">", COLOR_SUCCESS, Fade(COLOR_SUCCESS, 0.8f))) {
            if (net.ConnectToHost(serverIP, 12345)) {
                inGame = true;
            }
        }
        
        // 点击输入框
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            typing = CheckCollisionPointRec(mouse, inputBox);
        }
    }
    
    // 控制说明
    DrawRectangleRounded({50, (float)(screenHeight - 120), 320, 90}, 0.2f, 8, Fade(COLOR_PANEL, 0.8f));
    DrawText("Controls", 70, screenHeight - 105, 16, Fade(WHITE, 0.8f));
    DrawText("Host/1P: Arrow Keys or A/D", 70, screenHeight - 80, 14, Fade(WHITE, 0.5f));
    DrawText("Client: Z (left) / C (right)", 70, screenHeight - 60, 14, Fade(WHITE, 0.5f));
    DrawText("Launch: SPACE", 70, screenHeight - 40, 14, Fade(WHITE, 0.5f));
    
    // 快捷键提示
    DrawText("P - Pause  |  R - Restart", screenWidth - 250, screenHeight - 30, 14, Fade(WHITE, 0.4f));
}

// 绘制游戏HUD
void DrawGameHUD(int score, int lives, float gameTime, int winCount, 
                 GameMode mode, bool connected, bool remoteConnected) {
    
    const int screenWidth = GetScreenWidth();
    
    // 顶部状态栏背景
    DrawRectangle(0, 0, screenWidth, 55, Fade(COLOR_PANEL, 0.9f));
    DrawLine(0, 55, screenWidth, 55, Fade(WHITE, 0.1f));
    
    // 分数卡片
    DrawStatusCard(15, 8, 120, 40, "Score", TextFormat("%d", score), 
                   Fade(WHITE, 0.6f), COLOR_GOLD);
    
    // 生命卡片
    Color lifeColor = lives > 2 ? COLOR_SUCCESS : (lives > 1 ? COLOR_WARNING : COLOR_DANGER);
    DrawStatusCard(145, 8, 100, 40, "Lives", TextFormat("%d", lives),
                   Fade(WHITE, 0.6f), lifeColor);
    
    // 时间卡片
    DrawStatusCard(255, 8, 120, 40, "Time", TextFormat("%.1fs", gameTime),
                   Fade(WHITE, 0.6f), COLOR_ACCENT);
    
    // 剩余砖块
    DrawStatusCard(385, 8, 120, 40, "Bricks", TextFormat("%d", winCount),
                   Fade(WHITE, 0.6f), Fade(WHITE, 0.9f));
    
    // 网络状态
    if (mode != GameMode::SINGLE_PLAYER) {
        const char* role = (mode == GameMode::MULTIPLAYER_HOST) ? "HOST" : "CLIENT";
        DrawNetworkIndicator(screenWidth - 150, 18, connected, role);
        if (mode == GameMode::MULTIPLAYER_HOST) {
            if (remoteConnected) {
                DrawText("Client Connected", screenWidth - 150, 38, 12, COLOR_SUCCESS);
            } else {
                DrawText("Waiting for client...", screenWidth - 150, 38, 12, Fade(WHITE, 0.4f));
            }
        }
    } else {
        DrawText("Single Player", screenWidth - 130, 18, 16, Fade(WHITE, 0.5f));
    }
}

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    
    SetConfigFlags(FLAG_MSAA_4X_HINT);  // 抗锯齿
    InitWindow(screenWidth, screenHeight, "Breakout - Multiplayer Edition");
    InitChineseFont();
    SetTargetFPS(60);
    
    NetworkManager network;
    GameMode gameMode = GameMode::MENU;
    bool inGame = false;
    bool typing = false;
    char serverIP[32] = "127.0.0.1";
    
    std::vector<Ball> balls;
    Paddle paddle(340, 550, 120, 15);
    std::vector<Brick> bricks;
    PowerUpFactory powerUpFactory;
    std::vector<std::unique_ptr<PowerUp>> powerUps;
    ParticleSystem particleSystem;
    
    float remotePaddleX = 340;
    bool remoteConnected = false;
    
    Color brickColors[] = {
        Color{255, 80, 80, 255},    // 红
        Color{255, 140, 50, 255},   // 橙
        Color{255, 220, 50, 255},   // 黄
        Color{80, 200, 120, 255},   // 绿
        Color{80, 160, 255, 255}    // 蓝
    };
    
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 8; col++) {
            bricks.emplace_back(50.0f + col * 95, 100.0f + row * 35, 85.0f, 25.0f, brickColors[row]);
        }
    }
    
    balls.emplace_back(Vector2{400.0f, 530.0f}, Vector2{0.0f, 0.0f}, 10.0f);
    
    int score = 0, lives = 3, winCount = (int)bricks.size();
    bool gameOver = false, paused = false, victory = false;
    float gameTime = 0.0f;
    float gameSpeed = 1.0f;
    
    GameState gameState(&balls, &paddle, &lives, &score, &gameSpeed);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dropDist(0.0f, 1.0f);
    
    network.SetOnPaddleUpdate([&remotePaddleX, &remoteConnected](float x) {
        remotePaddleX = x;
        remoteConnected = true;
    });
    
    network.SetOnPlayerConnected([&remoteConnected]() {
        remoteConnected = true;
    });
    
    bool anyBallLaunched = false;
    float animTimer = 0.0f;
    
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        animTimer += deltaTime;
        
        // 处理键盘输入（IP输入）
        if (typing) {
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32 && key <= 125) || key == '.') {
                    int len = strlen(serverIP);
                    if (len < 31) {
                        serverIP[len] = (char)key;
                        serverIP[len+1] = '\0';
                    }
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                int len = strlen(serverIP);
                if (len > 0) serverIP[len-1] = '\0';
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
                typing = false;
            }
        }
        
        BeginDrawing();
        
        if (!inGame) {
            DrawMenu(gameMode, network, inGame, serverIP, typing);
        }
        else {
            // 输入处理
            if (IsKeyPressed(KEY_P) && !gameOver) paused = !paused;
            if (IsKeyPressed(KEY_R)) {
                balls.clear();
                balls.emplace_back(Vector2{400.0f, 530.0f}, Vector2{0.0f, 0.0f}, 10.0f);
                paddle = Paddle(340, 550, 120, 15);
                powerUps.clear();
                particleSystem.Clear();
                score = 0; lives = 3; gameOver = false; victory = false; paused = false; gameTime = 0.0f; gameSpeed = 1.0f;
                bricks.clear();
                for (int row = 0; row < 5; row++) 
                    for (int col = 0; col < 8; col++) 
                        bricks.emplace_back(50.0f + col * 95, 100.0f + row * 35, 85.0f, 25.0f, brickColors[row]);
                winCount = (int)bricks.size();
            }
            
            if (!gameOver && !paused) {
                gameTime += deltaTime;
                network.Update();
                
                float currentSpeed = IsKeyDown(KEY_LEFT_SHIFT) ? 14.0f : 9.0f;
                
                if (gameMode == GameMode::MULTIPLAYER_CLIENT) {
                    if (IsKeyDown(KEY_Z)) paddle.MoveLeft(currentSpeed);
                    if (IsKeyDown(KEY_C)) paddle.MoveRight(currentSpeed);
                } else {
                    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) paddle.MoveLeft(currentSpeed);
                    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) paddle.MoveRight(currentSpeed);
                }
                
                if (network.IsConnected()) {
                    network.SendPaddlePosition(paddle.GetRect().x);
                }
                
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
                
                for (auto& ball : balls) {
                    if (!ball.IsLaunched()) continue;
                    
                    ball.ApplyGravity();
                    ball.Move();
                    ball.BounceEdge(screenWidth, screenHeight);
                    ball.BouncePaddle(paddle.GetRect());
                    
                    for (auto& brick : bricks) {
                        if (brick.IsActive() && ball.CheckBrickCollision(brick.GetRect())) {
                            brick.SetActive(false);
                            score += CalculateScore(10, gameTime);
                            winCount--;
                            
                            Vector2 brickCenter = {
                                brick.GetRect().x + brick.GetRect().width / 2,
                                brick.GetRect().y + brick.GetRect().height / 2
                            };
                            particleSystem.EmitBrickBreak(brickCenter, brick.GetColor(), 15);
                            
                            if (dropDist(gen) < powerUpFactory.GetDropChance()) {
                                auto powerUp = powerUpFactory.CreateRandomPowerUp(brickCenter);
                                if (powerUp) powerUps.push_back(std::move(powerUp));
                            }
                            break;
                        }
                    }
                }
                
                balls.erase(std::remove_if(balls.begin(), balls.end(),
                    [screenHeight](Ball& b) { return b.GetPosition().y > screenHeight + 50; }), balls.end());
                
                if (balls.empty()) {
                    lives--;
                    if (lives <= 0) gameOver = true;
                    else balls.emplace_back(Vector2{400.0f, 530.0f}, Vector2{0.0f, 0.0f}, 10.0f);
                }
                
                if (winCount <= 0) {
                    gameOver = true;
                    victory = true;
                }
                
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
                
                particleSystem.Update();
            }
            
            // 绘制背景
            DrawGradientBackground();
            
            // 绘制边框
            DrawRectangle(0, 0, 5, screenHeight, Fade(COLOR_ACCENT, 0.3f));
            DrawRectangle(screenWidth - 5, 0, 5, screenHeight, Fade(COLOR_ACCENT, 0.3f));
            DrawRectangle(0, 0, screenWidth, 5, Fade(COLOR_ACCENT, 0.3f));
            
            // 绘制游戏对象
            for (auto& brick : bricks) brick.Draw();
            
            // 绘制远程挡板（带光晕）
            if (network.IsConnected() && remoteConnected && gameMode != GameMode::SINGLE_PLAYER) {
                float glow = sinf(animTimer * 3.0f) * 3.0f;
                DrawRectangleRounded({remotePaddleX, 550, 120, 15}, 0.3f, 8, Fade(GREEN, 0.15f));
                DrawRectangleRounded({remotePaddleX - glow/2, 550 - glow/2, 120 + glow, 15 + glow}, 
                                     0.3f, 8, Fade(GREEN, 0.1f));
                DrawRectangle(remotePaddleX, 550, 120, 15, Color{100, 220, 100, 220});
                DrawRectangleLines(remotePaddleX, 550, 120, 15, GREEN);
                DrawText("P2", remotePaddleX + 50, 530, 14, Fade(GREEN, 0.8f));
            }
            
            paddle.Draw();
            
            for (auto& ball : balls) ball.Draw();
            for (auto& pu : powerUps) pu->Draw(particleSystem);
            particleSystem.Draw();
            
            // 绘制HUD
            DrawGameHUD(score, lives, gameTime, winCount, gameMode, 
                       network.IsConnected(), remoteConnected);
            
            // 发射提示（带动画）
            if (!anyBallLaunched && !gameOver) {
                float alpha = (sinf(animTimer * 4.0f) + 1.0f) * 0.3f + 0.4f;
                DrawText("Press SPACE to Launch", screenWidth/2 - 120, 350, 22, Fade(COLOR_GOLD, alpha));
            }
            
            // 控制提示
            const char* controlHint = (gameMode == GameMode::MULTIPLAYER_CLIENT) ? 
                                      "Z/C to Move" : "Arrow Keys or A/D to Move";
            DrawText(controlHint, screenWidth - 160, screenHeight - 20, 12, Fade(WHITE, 0.3f));
            
            // 暂停界面
            if (paused && !gameOver) {
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.75f));
                DrawGlowText("PAUSED", screenWidth/2 - 70, screenHeight/2 - 40, 42, COLOR_GOLD);
                DrawText("Press P to Continue", screenWidth/2 - 95, screenHeight/2 + 20, 20, Fade(WHITE, 0.8f));
                DrawText("Press R to Restart", screenWidth/2 - 90, screenHeight/2 + 50, 18, Fade(WHITE, 0.5f));
            }
            
            // 游戏结束界面
            if (gameOver) {
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.85f));
                
                if (victory) {
                    DrawGlowText("VICTORY!", screenWidth/2 - 100, screenHeight/2 - 70, 48, COLOR_GOLD);
                    DrawText("You cleared all bricks!", screenWidth/2 - 120, screenHeight/2, 20, Fade(WHITE, 0.8f));
                } else {
                    DrawGlowText("GAME OVER", screenWidth/2 - 120, screenHeight/2 - 70, 48, COLOR_DANGER);
                }
                
                DrawText(TextFormat("Final Score: %d", score), screenWidth/2 - 70, screenHeight/2 + 40, 28, COLOR_GOLD);
                DrawText("Press R to Restart", screenWidth/2 - 90, screenHeight/2 + 90, 20, Fade(WHITE, 0.7f));
            }
        }
        
        EndDrawing();
    }
    
    network.Disconnect();
    CloseWindow();
    return 0;
}
