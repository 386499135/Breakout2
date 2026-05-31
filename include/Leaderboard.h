#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include <string>
#include <vector>
#include <ctime>

struct ScoreEntry {
    char name[32];
    int score;
    time_t timestamp;
};

class Leaderboard {
private:
    static const int MAX_ENTRIES = 10;
    ScoreEntry entries[MAX_ENTRIES];
    int count;
    std::string filename;
    void Load();
    void Save();
    
public:
    Leaderboard(const char* file);
    
    int AddScore(const char* name, int score);
    bool GetEntry(int rank, ScoreEntry& entry);
    int GetCount() const;
    bool CanEnter(int score) const;
};

#endif
