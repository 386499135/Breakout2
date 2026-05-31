#include <cmath>
#include "AsyncLoader.h"
#include "GameConstants.h"
#include <iostream>
#include <thread>
#include <chrono>

void AsyncLoader::StartSimulatedLoad(float duration) {
    if (isLoading.load()) return;
    
    isLoading = true;
    loadState = LoadState::LOADING;
    progress = 0.0f;
    {
        std::lock_guard<std::mutex> lock(messageMutex);
        loadMessage = "Loading assets...";
    }
    
    auto future = std::async(std::launch::async, [this, duration]() {
        const int steps = 100;
        float stepTime = duration / steps;
        
        for (int i = 0; i < steps; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds((int)(stepTime * 1000)));
            progress = (float)(i + 1) / steps;
            
            std::lock_guard<std::mutex> lock(messageMutex);
            if (progress < 0.3f) loadMessage = "Loading textures...";
            else if (progress < 0.6f) loadMessage = "Processing effects...";
            else if (progress < 0.9f) loadMessage = "Generating particles...";
            else loadMessage = "Finalizing...";
        }
        
        loadState = LoadState::COMPLETED;
        isLoading = false;
    });
    
    futures.push_back(std::move(future));
}

void AsyncLoader::Update(float deltaTime) {
    animTimer += deltaTime;
    
    for (auto it = futures.begin(); it != futures.end();) {
        if (!it->valid() || it->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            it = futures.erase(it);
        } else {
            ++it;
        }
    }
}

void AsyncLoader::DrawLoadingScreen(int screenWidth, int screenHeight) {
    if (!isLoading.load() && loadState != LoadState::COMPLETED) return;
    
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.8f));
    
    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;
    
    float rotation = animTimer * 360.0f;
    for (int i = 0; i < 12; i++) {
        float angle = i * 30.0f + rotation;
        float alpha = 0.2f + (i / 12.0f) * 0.8f;
        float x = centerX + cosf(angle * DEG2RAD) * 35;
        float y = centerY - 50 + sinf(angle * DEG2RAD) * 35;
        DrawCircle(x, y, 4, Fade(SKYBLUE, alpha));
    }
    
    DrawText("LOADING", centerX - 50, centerY - 100, 40, WHITE);
    
    std::string msg;
    {
        std::lock_guard<std::mutex> lock(messageMutex);
        msg = loadMessage;
    }
    DrawText(msg.c_str(), centerX - MeasureText(msg.c_str(), 16)/2, centerY + 20, 16, LIGHTGRAY);
    
    float barWidth = 300;
    float barHeight = 20;
    DrawRectangleRounded({centerX - barWidth/2, centerY + 50, barWidth, barHeight}, 0.5f, 8, COLOR_PANEL);
    DrawRectangleRounded({centerX - barWidth/2, centerY + 50, barWidth * progress, barHeight}, 0.5f, 8, COLOR_ACCENT);
    
    DrawText(TextFormat("%d%%", (int)(progress * 100)), centerX - 15, centerY + 52, 16, WHITE);
}

void AsyncLoader::Reset() {
    isLoading = false;
    loadState = LoadState::IDLE;
    progress = 0.0f;
    {
        std::lock_guard<std::mutex> lock(messageMutex);
        loadMessage.clear();
    }
    futures.clear();
}
