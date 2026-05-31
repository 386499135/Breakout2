#include "raylib.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const Color COLOR_BG = {20, 25, 35, 255};
const Color COLOR_GOLD = {255, 215, 0, 255};
const Color COLOR_ACCENT = {100, 150, 255, 255};
const Color COLOR_SILVER = {192, 192, 192, 255};
const Color COLOR_BRONZE = {205, 127, 50, 255};

// 砖块
struct Brick {
    Rectangle rect;
    bool active;
    Color color;
    int health;
};

// 球
struct Ball {
    Vector2 pos;
    Vector2 speed;
    float radius;
    bool launched;
};

// 粒子
struct Particle {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float life;
};

// 道具
struct PowerUp {
    Vector2 pos;
    float speed;
    bool active;
    int type;
    float lifetime;
};

// 存档数据
struct SaveData {
    int score;
    int lives;
    int level;
    float gameTime;
    std::vector<bool> brickStates;
    time_t saveTime;
};

// 全局变量
std::vector<Ball> balls;
std::vector<Brick> bricks;
std::vector<Particle> particles;
std::vector<PowerUp> powerUps;
Rectangle paddle;
int score = 0, lives = 5, currentLevel = 1, winCount = 0;
float gameSpeed = 1.0f, gameTime = 0.0f;
bool gameOver = false, victory = false, paused = false, anyBallLaunched = false;
float splitCooldown = 0.0f, saveTimer = 0.0f;
bool showSaveMsg = false, isLoading = false;
float loadProgress = 0.0f, loadAnimTimer = 0.0f;
std::string loadMessage = "";

// 排行榜
struct ScoreEntry {
    std::string name;
    int score;
    time_t timestamp;
};
std::vector<ScoreEntry> leaderboard;
bool showLeaderboard = false;

// 辅助函数
float Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// 存档功能
void SaveGame() {
    json save;
    save["score"] = score;
    save["lives"] = lives;
    save["level"] = currentLevel;
    save["gameTime"] = gameTime;
    save["saveTime"] = time(nullptr);
    
    json brickStates = json::array();
    for (const auto& brick : bricks) {
        brickStates.push_back(brick.active);
    }
    save["brickStates"] = brickStates;
    
    std::ofstream file("savegame.json");
    if (file.is_open()) {
        file << save.dump(2);
        file.close();
        showSaveMsg = true;
        saveTimer = 2.0f;
        std::cout << "游戏已保存！" << std::endl;
    }
}

bool LoadGame() {
    std::ifstream file("savegame.json");
    if (!file.is_open()) return false;
    
    try {
        json save;
        file >> save;
        score = save.value("score", 0);
        lives = save.value("lives", 5);
        currentLevel = save.value("level", 1);
        gameTime = save.value("gameTime", 0.0f);
        
        if (save.contains("brickStates")) {
            std::vector<bool> states;
            for (const auto& state : save["brickStates"]) {
                states.push_back(state.get<bool>());
            }
            for (size_t i = 0; i < bricks.size() && i < states.size(); i++) {
                bricks[i].active = states[i];
            }
            winCount = 0;
            for (const auto& brick : bricks) {
                if (brick.active) winCount++;
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

// 排行榜功能
void LoadLeaderboard() {
    leaderboard.clear();
    std::ifstream file("scores.txt");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line) && leaderboard.size() < 10) {
            size_t pos1 = line.find(' ');
            size_t pos2 = line.find(' ', pos1 + 1);
            if (pos1 != std::string::npos && pos2 != std::string::npos) {
                ScoreEntry entry;
                entry.name = line.substr(0, pos1);
                entry.score = std::stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
                entry.timestamp = std::stol(line.substr(pos2 + 1));
                leaderboard.push_back(entry);
            }
        }
        file.close();
    }
}

void SaveLeaderboard() {
    std::ofstream file("scores.txt");
    if (file.is_open()) {
        for (const auto& entry : leaderboard) {
            file << entry.name << " " << entry.score << " " << entry.timestamp << "\n";
        }
        file.close();
    }
}

void AddScoreToLeaderboard(int score) {
    ScoreEntry newEntry;
    newEntry.name = "Player";
    newEntry.score = score;
    newEntry.timestamp = time(nullptr);
    leaderboard.push_back(newEntry);
    
    std::sort(leaderboard.begin(), leaderboard.end(),
        [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
    
    if (leaderboard.size() > 10) leaderboard.resize(10);
    SaveLeaderboard();
}

// 异步加载模拟
void StartAsyncLoad() {
    isLoading = true;
    loadProgress = 0.0f;
    loadMessage = "Starting load...";
}

void UpdateAsyncLoad(float dt) {
    if (!isLoading) return;
    
    loadAnimTimer += dt;
    loadProgress += dt * 0.5f;
    
    if (loadProgress < 0.3f) loadMessage = "Loading textures...";
    else if (loadProgress < 0.6f) loadMessage = "Processing effects...";
    else if (loadProgress < 0.9f) loadMessage = "Generating particles...";
    else loadMessage = "Finalizing...";
    
    if (loadProgress >= 1.0f) {
        isLoading = false;
        loadProgress = 1.0f;
        loadMessage = "Complete!";
    }
}

void DrawLoadingScreen() {
    if (!isLoading) return;
    
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.85f));
    
    float centerX = SCREEN_WIDTH / 2.0f;
    float centerY = SCREEN_HEIGHT / 2.0f;
    
    float rotation = loadAnimTimer * 360.0f;
    for (int i = 0; i < 12; i++) {
        float angle = i * 30.0f + rotation;
        float alpha = 0.2f + (i / 12.0f) * 0.8f;
        float x = centerX + cos(angle * 3.14159f / 180.0f) * 35;
        float y = centerY - 50 + sin(angle * 3.14159f / 180.0f) * 35;
        DrawCircle(x, y, 4, Fade(SKYBLUE, alpha));
    }
    
    DrawText("LOADING", centerX - 50, centerY - 100, 40, WHITE);
    DrawText(loadMessage.c_str(), centerX - MeasureText(loadMessage.c_str(), 16)/2, centerY + 20, 16, LIGHTGRAY);
    
    float barWidth = 300;
    DrawRectangleRounded({centerX - barWidth/2, centerY + 50, barWidth, 20}, 0.5f, 8, {40,40,50,255});
    DrawRectangleRounded({centerX - barWidth/2, centerY + 50, barWidth * loadProgress, 20}, 0.5f, 8, COLOR_ACCENT);
    DrawText(TextFormat("%d%%", (int)(loadProgress * 100)), centerX - 15, centerY + 52, 16, WHITE);
}

// 关卡加载
void LoadLevel(int level) {
    bricks.clear();
    
    if (level == 1) {
        Color colors[] = {RED, ORANGE, YELLOW, GREEN, BLUE};
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 8; col++) {
                Brick b;
                b.rect = {(float)(50 + col * 95), (float)(80 + row * 35), 85.0f, 25.0f};
                b.active = true;
                b.color = colors[row];
                b.health = 1;
                bricks.push_back(b);
            }
        }
    } else if (level == 2) {
        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 9; col++) {
                int dist = abs(row - 3) + abs(col - 4);
                if (dist <= 3) {
                    Brick b;
                    b.rect = {(float)(30 + col * 85), (float)(80 + row * 32), 80.0f, 28.0f};
                    b.active = true;
                    b.health = 1;
                    if (dist == 0) b.color = RED;
                    else if (dist == 1) b.color = ORANGE;
                    else if (dist == 2) b.color = YELLOW;
                    else b.color = GREEN;
                    bricks.push_back(b);
                }
            }
        }
    } else {
        for (int row = 0; row < 6; row++) {
            for (int col = 0; col < 10; col++) {
                if ((row == 0 && (col == 0 || col == 1 || col == 8 || col == 9)) ||
                    (row == 1 && (col == 0 || col == 1 || col == 8 || col == 9)) ||
                    (row >= 2 && row <= 4 && col >= 2 && col <= 7)) {
                    Brick b;
                    b.rect = {(float)(25 + col * 76), (float)(80 + row * 30), 72.0f, 26.0f};
                    b.active = true;
                    b.health = 1;
                    if (row == 0) b.color = RED;
                    else if (row == 1) b.color = ORANGE;
                    else b.color = YELLOW;
                    bricks.push_back(b);
                }
            }
        }
    }
    
    winCount = 0;
    for (const auto& brick : bricks) {
        if (brick.active) winCount++;
    }
}

// 重置游戏
void ResetGame() {
    balls.clear();
    Ball ball;
    ball.pos = {400, 530};
    ball.speed = {0, 0};
    ball.radius = 10;
    ball.launched = false;
    balls.push_back(ball);
    
    paddle = {340, 550, 120, 15};
    powerUps.clear();
    particles.clear();
    score = 0;
    lives = 5;
    gameSpeed = 1.0f;
    gameTime = 0.0f;
    gameOver = false;
    victory = false;
    paused = false;
    splitCooldown = 0.0f;
    anyBallLaunched = false;
    
    LoadLevel(currentLevel);
}

// 下一关
void NextLevel() {
    if (currentLevel < 3) {
        currentLevel++;
        ResetGame();
    } else {
        victory = true;
        gameOver = true;
        AddScoreToLeaderboard(score);
    }
}

// 粒子效果
void AddParticles(Vector2 pos, Color color) {
    for (int i = 0; i < 12; i++) {
        Particle p;
        p.pos = pos;
        p.vel = {(float)((rand() % 100 - 50) / 8.0f), (float)((rand() % 100 - 50) / 8.0f) - 2};
        p.color = color;
        p.life = 0.8f;
        particles.push_back(p);
    }
}

// 道具生成
void SpawnPowerUp(Vector2 pos) {
    if ((rand() % 100) < 25) {
        PowerUp p;
        p.pos = pos;
        p.speed = 2.5f;
        p.active = true;
        p.type = rand() % 4;
        p.lifetime = 10.0f;
        powerUps.push_back(p);
    }
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "BREAKOUT - Complete Edition");
    SetTargetFPS(60);
    srand(time(nullptr));
    
    LoadLevel(1);
    ResetGame();
    LoadLeaderboard();
    
    enum MenuState { MAIN_MENU, PLAYING, PAUSED, GAME_OVER_MENU };
    MenuState menuState = MAIN_MENU;
    int menuSelect = 0;
    float frameTimer = 0.0f;
    int currentFPS = 60;
    
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.033f) dt = 0.033f;
        
        UpdateAsyncLoad(dt);
        
        frameTimer += dt;
        if (frameTimer >= 0.5f) {
            currentFPS = GetFPS();
            frameTimer = 0.0f;
        }
        
        if (saveTimer > 0) {
            saveTimer -= dt;
            if (saveTimer <= 0) showSaveMsg = false;
        }
        if (splitCooldown > 0) splitCooldown -= dt;
        
        // 主菜单
        if (menuState == MAIN_MENU) {
            if (IsKeyPressed(KEY_UP)) menuSelect = (menuSelect - 1 + 5) % 5;
            if (IsKeyPressed(KEY_DOWN)) menuSelect = (menuSelect + 1) % 5;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (menuSelect == 0) {
                    currentLevel = 1;
                    ResetGame();
                    menuState = PLAYING;
                } else if (menuSelect == 1) {
                    if (LoadGame()) {
                        ResetGame();
                        menuState = PLAYING;
                    }
                } else if (menuSelect == 2) {
                    showLeaderboard = true;
                } else if (menuSelect == 3) {
                    StartAsyncLoad();
                } else if (menuSelect == 4) {
                    break;
                }
            }
            if (IsKeyPressed(KEY_ESCAPE) && showLeaderboard) {
                showLeaderboard = false;
            }
        }
        // 游戏中
        else if (menuState == PLAYING && !gameOver) {
            gameTime += dt;
            
            if (IsKeyPressed(KEY_P)) menuState = PAUSED;
            if (IsKeyPressed(KEY_R)) { ResetGame(); menuState = PLAYING; }
            if (IsKeyPressed(KEY_F5)) SaveGame();
            if (IsKeyPressed(KEY_L)) showLeaderboard = !showLeaderboard;
            
            // 分裂球
            if (IsKeyPressed(KEY_S) && splitCooldown <= 0 && !balls.empty()) {
                std::vector<Ball> newBalls;
                for (size_t bi = 0; bi < balls.size(); bi++) {
                    if (balls[bi].launched) {
                        float speedVal = sqrt(balls[bi].speed.x * balls[bi].speed.x + balls[bi].speed.y * balls[bi].speed.y);
                        for (int i = 0; i < 2; i++) {
                            Ball nb;
                            nb.pos = balls[bi].pos;
                            nb.radius = balls[bi].radius * 0.7f;
                            if (nb.radius < 5) nb.radius = 5;
                            nb.launched = true;
                            float angle = (i * 180 + (rand() % 60)) * 3.14159f / 180.0f;
                            nb.speed.x = cos(angle) * speedVal;
                            nb.speed.y = sin(angle) * speedVal;
                            newBalls.push_back(nb);
                        }
                    }
                }
                for (size_t i = 0; i < newBalls.size(); i++) {
                    balls.push_back(newBalls[i]);
                }
                splitCooldown = 2.0f;
            }
            
            float pspeed = (IsKeyDown(KEY_LEFT_SHIFT) ? 28.0f : 18.0f) * gameSpeed;
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
                paddle.x -= pspeed;
                if (paddle.x < 5) paddle.x = 5;
            }
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
                paddle.x += pspeed;
                if (paddle.x + paddle.width > SCREEN_WIDTH - 5) paddle.x = SCREEN_WIDTH - paddle.width - 5;
            }
            
            anyBallLaunched = false;
            for (size_t bi = 0; bi < balls.size(); bi++) {
                if (!balls[bi].launched) {
                    balls[bi].pos.x = paddle.x + paddle.width / 2;
                    balls[bi].pos.y = paddle.y - balls[bi].radius - 5;
                } else {
                    anyBallLaunched = true;
                }
            }
            
            if (!anyBallLaunched && IsKeyPressed(KEY_SPACE)) {
                balls[0].launched = true;
                balls[0].speed.x = 5.5f;
                balls[0].speed.y = -7.5f;
                balls[0].pos.x = paddle.x + paddle.width / 2;
            }
            
            for (size_t bi = 0; bi < balls.size(); bi++) {
                if (!balls[bi].launched) continue;
                
                balls[bi].pos.x += balls[bi].speed.x;
                balls[bi].pos.y += balls[bi].speed.y;
                balls[bi].speed.y += 0.06f;
                
                if (balls[bi].pos.x - balls[bi].radius <= 5) {
                    balls[bi].pos.x = 5 + balls[bi].radius;
                    balls[bi].speed.x = fabs(balls[bi].speed.x);
                }
                if (balls[bi].pos.x + balls[bi].radius >= SCREEN_WIDTH - 5) {
                    balls[bi].pos.x = SCREEN_WIDTH - 5 - balls[bi].radius;
                    balls[bi].speed.x = -fabs(balls[bi].speed.x);
                }
                if (balls[bi].pos.y - balls[bi].radius <= 5) {
                    balls[bi].pos.y = 5 + balls[bi].radius;
                    balls[bi].speed.y = fabs(balls[bi].speed.y);
                }
                
                if (balls[bi].speed.y > 0 && 
                    balls[bi].pos.y + balls[bi].radius >= paddle.y &&
                    balls[bi].pos.x + balls[bi].radius > paddle.x && 
                    balls[bi].pos.x - balls[bi].radius < paddle.x + paddle.width) {
                    
                    float hit = (balls[bi].pos.x - (paddle.x + paddle.width/2)) / (paddle.width/2);
                    hit = Clamp(hit, -1.0f, 1.0f);
                    float spd = sqrt(balls[bi].speed.x * balls[bi].speed.x + balls[bi].speed.y * balls[bi].speed.y);
                    float angle = (90 - hit * 55) * 3.14159f / 180.0f;
                    balls[bi].speed.x = spd * cos(angle);
                    balls[bi].speed.y = -spd * fabs(sin(angle));
                    balls[bi].pos.y = paddle.y - balls[bi].radius;
                }
                
                for (size_t bri = 0; bri < bricks.size(); bri++) {
                    if (!bricks[bri].active) continue;
                    if (CheckCollisionCircleRec(balls[bi].pos, balls[bi].radius, bricks[bri].rect)) {
                        bricks[bri].active = false;
                        score += 10;
                        winCount--;
                        AddParticles({bricks[bri].rect.x + bricks[bri].rect.width/2, 
                                      bricks[bri].rect.y + bricks[bri].rect.height/2}, bricks[bri].color);
                        SpawnPowerUp({bricks[bri].rect.x + bricks[bri].rect.width/2, 
                                      bricks[bri].rect.y + bricks[bri].rect.height/2});
                        
                        float overlapL = balls[bi].pos.x + balls[bi].radius - bricks[bri].rect.x;
                        float overlapR = bricks[bri].rect.x + bricks[bri].rect.width - (balls[bi].pos.x - balls[bi].radius);
                        float overlapT = balls[bi].pos.y + balls[bi].radius - bricks[bri].rect.y;
                        float overlapB = bricks[bri].rect.y + bricks[bri].rect.height - (balls[bi].pos.y - balls[bi].radius);
                        
                        float minX = overlapL < overlapR ? overlapL : overlapR;
                        float minY = overlapT < overlapB ? overlapT : overlapB;
                        
                        if (minX < minY) balls[bi].speed.x *= -1;
                        else balls[bi].speed.y *= -1;
                        break;
                    }
                }
            }
            
            for (size_t i = 0; i < balls.size();) {
                if (balls[i].pos.y > SCREEN_HEIGHT + 100) {
                    balls.erase(balls.begin() + i);
                } else {
                    i++;
                }
            }
            
            if (balls.empty()) {
                lives--;
                if (lives <= 0) {
                    gameOver = true;
                    victory = false;
                    AddScoreToLeaderboard(score);
                } else {
                    Ball nb;
                    nb.pos = {400, 530};
                    nb.speed = {0, 0};
                    nb.radius = 10;
                    nb.launched = false;
                    balls.push_back(nb);
                }
            }
            
            if (winCount <= 0 && !gameOver) {
                NextLevel();
            }
            
            for (size_t i = 0; i < powerUps.size();) {
                powerUps[i].pos.y += powerUps[i].speed;
                powerUps[i].lifetime -= dt;
                if (powerUps[i].pos.y > SCREEN_HEIGHT + 50 || powerUps[i].lifetime <= 0) {
                    powerUps.erase(powerUps.begin() + i);
                    continue;
                }
                if (CheckCollisionPointRec(powerUps[i].pos, paddle)) {
                    if (powerUps[i].type == 0) gameSpeed = 1.5f;
                    else if (powerUps[i].type == 1) gameSpeed = 0.6f;
                    else if (powerUps[i].type == 2) paddle.width = 180;
                    else if (powerUps[i].type == 3) lives++;
                    powerUps.erase(powerUps.begin() + i);
                } else {
                    i++;
                }
            }
            
            for (size_t i = 0; i < particles.size();) {
                particles[i].pos.x += particles[i].vel.x;
                particles[i].pos.y += particles[i].vel.y;
                particles[i].vel.y += 0.2f;
                particles[i].life -= dt;
                if (particles[i].life <= 0) {
                    particles.erase(particles.begin() + i);
                } else {
                    i++;
                }
            }
            
            static float speedTimer = 0;
            static float paddleTimer = 0;
            if (gameSpeed != 1.0f) {
                speedTimer += dt;
                if (speedTimer > 8.0f) {
                    gameSpeed = 1.0f;
                    speedTimer = 0;
                }
            }
            if (paddle.width > 120) {
                paddleTimer += dt;
                if (paddleTimer > 8.0f) {
                    paddle.width = 120;
                    paddleTimer = 0;
                }
            }
        }
        else if (menuState == PAUSED) {
            if (IsKeyPressed(KEY_P)) menuState = PLAYING;
            if (IsKeyPressed(KEY_ESCAPE)) menuState = MAIN_MENU;
        }
        else if (menuState == GAME_OVER_MENU || (menuState == PLAYING && gameOver)) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                currentLevel = 1;
                ResetGame();
                menuState = PLAYING;
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                menuState = MAIN_MENU;
            }
        }
        
        // 渲染
        BeginDrawing();
        ClearBackground(COLOR_BG);
        
        if (menuState == MAIN_MENU) {
            DrawText("BREAKOUT", SCREEN_WIDTH/2 - 100, 60, 48, COLOR_GOLD);
            DrawText("Complete Edition", SCREEN_WIDTH/2 - 80, 110, 20, Fade(WHITE, 0.6f));
            
            const char* items[] = {"New Game", "Continue", "Leaderboard", "Test Load", "Exit"};
            for (int i = 0; i < 5; i++) {
                Color c = (menuSelect == i) ? COLOR_GOLD : Fade(WHITE, 0.7f);
                DrawText(TextFormat("%s %s", (menuSelect == i) ? ">" : " ", items[i]), 
                         SCREEN_WIDTH/2 - 50, 170 + i * 45, 24, c);
            }
            
            std::ifstream check("savegame.json");
            if (check.is_open()) {
                DrawText("Save file found!", SCREEN_WIDTH/2 - 60, 420, 14, Fade(GREEN, 0.7f));
                check.close();
            }
            
            DrawText("UP/DOWN: Navigate | ENTER: Select", 20, SCREEN_HEIGHT - 30, 14, Fade(WHITE, 0.4f));
            DrawText("S:Split | P:Pause | R:Restart | F5:Save | L:Rank", SCREEN_WIDTH - 320, SCREEN_HEIGHT - 30, 14, Fade(WHITE, 0.4f));
        }
        else if (menuState == PLAYING) {
            DrawRectangle(0, 0, 5, SCREEN_HEIGHT, {60,65,85,255});
            DrawRectangle(SCREEN_WIDTH-5, 0, 5, SCREEN_HEIGHT, {60,65,85,255});
            DrawRectangle(0, 0, SCREEN_WIDTH, 5, {60,65,85,255});
            
            for (size_t i = 0; i < bricks.size(); i++) {
                if (bricks[i].active) {
                    DrawRectangleRec(bricks[i].rect, bricks[i].color);
                    DrawRectangleLinesEx(bricks[i].rect, 1, Fade(WHITE, 0.5f));
                }
            }
            
            DrawRectangleRounded(paddle, 0.3f, 8, BLUE);
            DrawRectangleRoundedLines(paddle, 0.3f, 8, 2, SKYBLUE);
            
            for (size_t i = 0; i < balls.size(); i++) {
                DrawCircleV(balls[i].pos, balls[i].radius, RED);
                DrawCircleGradient(balls[i].pos.x, balls[i].pos.y, balls[i].radius-2, ORANGE, RED);
            }
            
            for (size_t i = 0; i < powerUps.size(); i++) {
                Color col = powerUps[i].type == 0 ? GREEN : (powerUps[i].type == 1 ? SKYBLUE : (powerUps[i].type == 2 ? YELLOW : RED));
                DrawCircleV(powerUps[i].pos, 18, Fade(col, 0.3f));
                DrawRectangleRounded({powerUps[i].pos.x-15, powerUps[i].pos.y-15, 30, 30}, 0.3f, 8, col);
                const char* txt = powerUps[i].type == 0 ? "SPD" : (powerUps[i].type == 1 ? "SLW" : (powerUps[i].type == 2 ? "WIDE" : "LIFE"));
                DrawText(txt, powerUps[i].pos.x - 10, powerUps[i].pos.y - 6, 12, WHITE);
            }
            
            for (size_t i = 0; i < particles.size(); i++) {
                DrawCircleV(particles[i].pos, 2, Fade(particles[i].color, particles[i].life));
            }
            
            DrawRectangleRounded({10,10,120,40}, 0.2f, 8, {30,35,50,220});
            DrawText(TextFormat("Score: %d", score), 20, 18, 20, WHITE);
            
            DrawRectangleRounded({SCREEN_WIDTH-130,10,120,40}, 0.2f, 8, {30,35,50,220});
            DrawText(TextFormat("FPS: %d", currentFPS), SCREEN_WIDTH-120, 18, 20, LIME);
            
            DrawRectangleRounded({10,55,100,40}, 0.2f, 8, {30,35,50,220});
            Color lc = lives > 3 ? GREEN : (lives > 1 ? YELLOW : RED);
            DrawText(TextFormat("Lives: %d", lives), 20, 63, 20, lc);
            
            DrawRectangleRounded({120,55,100,40}, 0.2f, 8, {30,35,50,220});
            DrawText(TextFormat("Balls: %d", (int)balls.size()), 130, 63, 20, balls.size() > 3 ? PURPLE : WHITE);
            
            DrawRectangleRounded({230,55,120,40}, 0.2f, 8, {30,35,50,220});
            DrawText(TextFormat("Level: %d/3", currentLevel), 240, 63, 20, COLOR_ACCENT);
            
            DrawRectangleRounded({360,55,120,40}, 0.2f, 8, {30,35,50,220});
            DrawText(TextFormat("Time: %.0fs", gameTime), 370, 63, 16, Fade(WHITE, 0.8f));
            
            if (splitCooldown > 0) {
                DrawRectangleRounded({490,55,120,40}, 0.2f, 8, {30,35,50,220});
                DrawText(TextFormat("Split: %.1f", splitCooldown), 500, 63, 16, Fade(RED, 0.8f));
            }
            
            if (gameSpeed != 1.0f) {
                DrawText(gameSpeed > 1.0f ? ">>> SPEED BOOST <<<" : "<<< SLOW MOTION >>>", 
                         SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT - 60, 16, gameSpeed > 1.0f ? GREEN : SKYBLUE);
            }
            
            if (paddle.width > 130) {
                DrawText("WIDE PADDLE!", SCREEN_WIDTH/2 - 50, SCREEN_HEIGHT - 80, 16, YELLOW);
            }
            
            if (!anyBallLaunched && !gameOver) {
                float alpha = (sinf(GetTime() * 4) + 1) * 0.3f + 0.4f;
                DrawText("PRESS SPACE TO LAUNCH", SCREEN_WIDTH/2 - 120, 350, 24, Fade(YELLOW, alpha));
            }
            
            if (showSaveMsg) {
                DrawText("GAME SAVED!", SCREEN_WIDTH/2 - 50, SCREEN_HEIGHT/2 - 50, 28, GREEN);
            }
            
            DrawText("S:Split | P:Pause | R:Reset | F5:Save | L:Rank", 20, SCREEN_HEIGHT - 25, 14, Fade(WHITE, 0.5f));
            
            if (gameOver) {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.85f));
                if (victory) {
                    DrawText("VICTORY!", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 40, 48, COLOR_GOLD);
                } else {
                    DrawText("GAME OVER", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 40, 48, RED);
                }
                DrawText(TextFormat("Score: %d", score), SCREEN_WIDTH/2 - 50, SCREEN_HEIGHT/2 + 10, 28, YELLOW);
                DrawText("Press ENTER to Play Again", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 + 80, 20, GREEN);
            }
            
            if (showLeaderboard) {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.9f));
                DrawText("LEADERBOARD", SCREEN_WIDTH/2 - 70, 50, 32, COLOR_GOLD);
                DrawText("RANK", 150, 100, 20, Fade(WHITE, 0.6f));
                DrawText("NAME", 250, 100, 20, Fade(WHITE, 0.6f));
                DrawText("SCORE", 500, 100, 20, Fade(WHITE, 0.6f));
                DrawLine(150, 125, 700, 125, Fade(WHITE, 0.3f));
                
                for (size_t i = 0; i < leaderboard.size(); i++) {
                    int y = 140 + i * 35;
                    Color c = WHITE;
                    if (i == 0) c = COLOR_GOLD;
                    else if (i == 1) c = COLOR_SILVER;
                    else if (i == 2) c = COLOR_BRONZE;
                    DrawText(TextFormat("#%d", (int)(i+1)), 150, y, 22, c);
                    DrawText(leaderboard[i].name.c_str(), 250, y, 22, c);
                    DrawText(TextFormat("%d", leaderboard[i].score), 500, y, 22, c);
                }
                
                DrawText("Press L to close", SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT - 30, 16, Fade(WHITE, 0.5f));
            }
        }
        else if (menuState == PAUSED) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));
            DrawText("PAUSED", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 30, 48, YELLOW);
            DrawText("Press P to Resume", SCREEN_WIDTH/2 - 90, SCREEN_HEIGHT/2 + 20, 20, WHITE);
            DrawText("Press ESC to Menu", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 50, 16, Fade(WHITE, 0.7f));
        }
        
        DrawLoadingScreen();
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}
