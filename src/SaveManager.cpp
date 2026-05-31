#include "SaveManager.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

SaveManager::SaveManager() {
    saveFile = "savegame.json";
    levelsFile = "levels/levels.json";
}

bool SaveManager::SaveGame(int score, int lives, int level, float gameTime, float gameSpeed, const std::vector<Brick>& bricks) {
    json save;
    save["score"] = score;
    save["lives"] = lives;
    save["currentLevel"] = level;
    save["gameTime"] = gameTime;
    save["gameSpeed"] = gameSpeed;
    save["saveTime"] = time(nullptr);
    
    json brickStates = json::array();
    for (const auto& brick : bricks) {
        brickStates.push_back(brick.IsActive());
    }
    save["brickStates"] = brickStates;
    
    std::ofstream file(saveFile);
    if (!file.is_open()) return false;
    file << save.dump(2);
    file.close();
    
    std::cout << "游戏已保存！关卡 " << level << " 分数 " << score << std::endl;
    return true;
}

bool SaveManager::HasSaveGame() {
    std::ifstream file(saveFile);
    return file.is_open();
}

bool SaveManager::LoadGame(SaveData& data) {
    std::ifstream file(saveFile);
    if (!file.is_open()) return false;
    
    try {
        json save;
        file >> save;
        data.score = save.value("score", 0);
        data.lives = save.value("lives", 5);
        data.currentLevel = save.value("currentLevel", 1);
        data.gameTime = save.value("gameTime", 0.0f);
        data.gameSpeed = save.value("gameSpeed", 1.0f);
        data.saveTime = save.value("saveTime", 0);
        
        if (save.contains("brickStates")) {
            data.brickStates.clear();
            for (const auto& state : save["brickStates"]) {
                data.brickStates.push_back(state.get<bool>());
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

void SaveManager::DeleteSave() {
    std::remove(saveFile.c_str());
}

std::string SaveManager::GetSaveTimeString() {
    SaveData data;
    if (LoadGame(data) && data.saveTime > 0) {
        char buffer[64];
        struct tm* timeinfo = localtime(&data.saveTime);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        return std::string(buffer);
    }
    return "";
}

std::vector<Brick> SaveManager::LoadLevel(int level, const std::vector<bool>* savedStates) {
    std::vector<Brick> bricks;
    
    if (level == 1) {
        Color colors[] = {{255,80,80,255}, {255,140,50,255}, {255,220,50,255}, {80,200,120,255}, {80,160,255,255}};
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 8; col++) {
                bricks.emplace_back(50.0f + col * 95, 80.0f + row * 35, 85.0f, 25.0f, colors[row]);
            }
        }
    } else if (level == 2) {
        Color colors[] = {{255,80,80,255}, {255,140,50,255}, {255,220,50,255}, {80,200,120,255}};
        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 9; col++) {
                int dist = abs(row - 3) + abs(col - 4);
                if (dist <= 3) {
                    Color color;
                    if (dist == 0) color = colors[0];
                    else if (dist == 1) color = colors[1];
                    else if (dist == 2) color = colors[2];
                    else color = colors[3];
                    bricks.emplace_back(30.0f + col * 85, 80.0f + row * 32, 80.0f, 28.0f, color);
                }
            }
        }
    } else if (level == 3) {
        Color colors[] = {{255,80,80,255}, {255,140,50,255}, {255,220,50,255}};
        for (int row = 0; row < 6; row++) {
            for (int col = 0; col < 10; col++) {
                if (row == 0 && (col == 0 || col == 1 || col == 8 || col == 9)) {
                    bricks.emplace_back(25.0f + col * 76, 80.0f + row * 30, 72.0f, 26.0f, colors[0]);
                } else if (row == 1 && (col == 0 || col == 1 || col == 8 || col == 9)) {
                    bricks.emplace_back(25.0f + col * 76, 80.0f + row * 30, 72.0f, 26.0f, colors[1]);
                } else if (row >= 2 && row <= 4 && col >= 2 && col <= 7) {
                    bricks.emplace_back(25.0f + col * 76, 80.0f + row * 30, 72.0f, 26.0f, colors[2]);
                }
            }
        }
    }
    
    if (savedStates && !savedStates->empty()) {
        for (size_t i = 0; i < bricks.size() && i < savedStates->size(); i++) {
            bricks[i].SetActive((*savedStates)[i]);
        }
    }
    
    return bricks;
}

int SaveManager::GetLevelCount() const {
    return 3;
}

std::string SaveManager::GetLevelName(int level) const {
    switch(level) {
        case 1: return "Classic";
        case 2: return "Diamond";
        case 3: return "Castle";
        default: return "Unknown";
    }
}
