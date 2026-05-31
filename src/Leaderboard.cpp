#include "Leaderboard.h"
#include <cstdio>
#include <cstring>
#include <iostream>

Leaderboard::Leaderboard(const char* file) : count(0), filename(file) {
    Load();
}

void Leaderboard::Load() {
    FILE* f = fopen(filename.c_str(), "r");
    if (f) {
        count = 0;
        while (count < MAX_ENTRIES && 
               fscanf(f, "%31s %d %ld", entries[count].name, &entries[count].score, &entries[count].timestamp) == 3) {
            count++;
        }
        fclose(f);
    }
}

void Leaderboard::Save() {
    FILE* f = fopen(filename.c_str(), "w");
    if (f) {
        for (int i = 0; i < count; i++) {
            fprintf(f, "%s %d %ld\n", entries[i].name, entries[i].score, entries[i].timestamp);
        }
        fclose(f);
    }
}

int Leaderboard::AddScore(const char* name, int score) {
    if (count >= MAX_ENTRIES && score <= entries[count - 1].score) return 0;
    
    ScoreEntry newEntry;
    strncpy(newEntry.name, name, 31);
    newEntry.name[31] = '\0';
    newEntry.score = score;
    newEntry.timestamp = time(nullptr);
    
    int pos = 0;
    while (pos < count && entries[pos].score >= score) pos++;
    
    if (count < MAX_ENTRIES) count++;
    for (int i = count - 1; i > pos; i--) {
        entries[i] = entries[i - 1];
    }
    entries[pos] = newEntry;
    Save();
    return pos + 1;
}

bool Leaderboard::GetEntry(int rank, ScoreEntry& entry) {
    if (rank > 0 && rank <= count) {
        entry = entries[rank - 1];
        return true;
    }
    return false;
}

int Leaderboard::GetCount() const {
    return count;
}

bool Leaderboard::CanEnter(int score) const {
    return count < MAX_ENTRIES || score > entries[count - 1].score;
}
