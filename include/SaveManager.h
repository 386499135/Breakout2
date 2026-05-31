#ifndef SAVE_MANAGER_H
#define SAVE_MANAGER_H

#include "raylib.h"
#include "Brick.h"
#include <string>
#include <vector>
#include <ctime>

struct SaveData {
    int score = 0;
    int lives = 5;
    int currentLevel = 1;
    float gameTime = 0.0f;
    float gameSpeed = 1.0f;
    std::vector<bool> brickStates;
    time_t saveTime = 0;
};

class SaveManager {
private:
    std::string saveFile;
    std::string levelsFile;
    
public:
    SaveManager();
    
    bool SaveGame(int score, int lives, int level, float gameTime, float gameSpeed, const std::vector<Brick>& bricks);
    bool HasSaveGame();
    bool LoadGame(SaveData& data);
    void DeleteSave();
    std::string GetSaveTimeString();
    
    std::vector<Brick> LoadLevel(int level, const std::vector<bool>* savedStates = nullptr);
    int GetLevelCount() const;
    std::string GetLevelName(int level) const;
};

#endif
