#ifndef ASYNC_LOADER_H
#define ASYNC_LOADER_H

#include "raylib.h"
#include <string>
#include <future>
#include <atomic>
#include <mutex>
#include <vector>

enum class LoadState {
    IDLE, LOADING, COMPLETED, FAILED
};

class AsyncLoader {
private:
    std::atomic<bool> isLoading{false};
    std::atomic<LoadState> loadState{LoadState::IDLE};
    std::atomic<float> progress{0.0f};
    std::string loadMessage;
    std::mutex messageMutex;
    float animTimer = 0.0f;
    std::vector<std::future<void>> futures;
    
public:
    void StartSimulatedLoad(float duration = 3.0f);
    void Update(float deltaTime);
    void DrawLoadingScreen(int screenWidth, int screenHeight);
    void Reset();
    
    bool IsLoading() const { return isLoading.load(); }
    LoadState GetState() const { return loadState.load(); }
    float GetProgress() const { return progress.load(); }
};

#endif
