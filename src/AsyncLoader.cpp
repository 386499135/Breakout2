#include "AsyncLoader.h"
#include <iostream>
#include <cmath>

// ============== TextureCache 实现 ==============
Texture2D TextureCache::GetOrLoad(const std::string& path) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = cache.find(path);
    if (it != cache.end()) {
        return it->second;
    }
    
    // 加载纹理（注意：raylib纹理加载需要线程安全）
    Image img = LoadImage(path.c_str());
    if (img.data != nullptr) {
        Texture2D texture = LoadTextureFromImage(img);
        UnloadImage(img);
        cache[path] = texture;
        std::cout << "[TextureCache] Loaded: " << path << std::endl;
        return texture;
    }
    
    return {0};
}

bool TextureCache::Preload(const std::string& path) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    if (cache.find(path) != cache.end()) {
        return true;
    }
    
    Image img = LoadImage(path.c_str());
    if (img.data != nullptr) {
        Texture2D texture = LoadTextureFromImage(img);
        UnloadImage(img);
        cache[path] = texture;
        std::cout << "[TextureCache] Preloaded: " << path << std::endl;
        return true;
    }
    
    return false;
}

bool TextureCache::Has(const std::string& path) const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return cache.find(path) != cache.end();
}

Texture2D TextureCache::Get(const std::string& path) const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = cache.find(path);
    if (it != cache.end()) {
        return it->second;
    }
    return {0};
}

void TextureCache::Clear() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    for (auto& pair : cache) {
        UnloadTexture(pair.second);
    }
    cache.clear();
}

size_t TextureCache::Size() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return cache.size();
}

// ============== AsyncLoader 实现 ==============
void AsyncLoader::StartSimulatedLoad(float duration) {
    if (m_isLoading.load()) return;
    
    m_isLoading = true;
    m_loadState = LoadState::LOADING;
    m_progress = 0.0f;
    m_completedTasks = 0;
    m_totalTasks = 1;
    m_loadMessage = "Loading assets...";
    
    // 使用 std::async 创建工作线程
    auto future = std::async(std::launch::async, [this, duration]() {
        std::cout << "[Worker Thread " << std::this_thread::get_id() 
                  << "] Starting simulated load, duration: " << duration << "s" << std::endl;
        
        const int steps = 100;
        const float stepTime = duration / steps;
        
        for (int i = 0; i < steps; i++) {
            // 模拟耗时操作
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(stepTime * 1000)));
            
            // 线程安全地更新进度
            float progress = static_cast<float>(i + 1) / steps;
            m_progress.store(progress);
            
            // 更新消息
            {
                std::lock_guard<std::mutex> lock(m_stateMutex);
                if (progress < 0.3f) {
                    m_loadMessage = "Loading textures...";
                } else if (progress < 0.6f) {
                    m_loadMessage = "Processing shaders...";
                } else if (progress < 0.9f) {
                    m_loadMessage = "Generating particles...";
                } else {
                    m_loadMessage = "Finalizing...";
                }
            }
        }
        
        std::cout << "[Worker Thread " << std::this_thread::get_id() 
                  << "] Load complete!" << std::endl;
        
        m_completedTasks.store(1);
        m_loadState.store(LoadState::COMPLETED);
        m_isLoading.store(false);
    });
    
    // 存储 future 以防销毁
    std::lock_guard<std::mutex> lock(m_resultMutex);
    m_futures.push_back(std::move(future));
}

void AsyncLoader::StartTextureLoad(const std::string& path) {
    if (m_isLoading.load()) return;
    
    m_isLoading = true;
    m_loadState = LoadState::LOADING;
    m_progress = 0.0f;
    m_completedTasks = 0;
    m_totalTasks = 1;
    
    // 使用 std::async 加载纹理
    auto future = std::async(std::launch::async, [this, path]() {
        std::cout << "[Worker Thread " << std::this_thread::get_id() 
                  << "] Loading texture: " << path << std::endl;
        
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_loadMessage = "Loading " + path + "...";
        }
        
        m_progress.store(0.2f);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // 在线程中加载纹理
        bool success = m_textureCache.Preload(path);
        m_progress.store(0.8f);
        
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            if (success) {
                m_loadMessage = "Texture loaded!";
            } else {
                m_loadMessage = "Failed to load texture!";
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        m_progress.store(1.0f);
        
        m_completedTasks.store(1);
        m_loadState.store(success ? LoadState::COMPLETED : LoadState::FAILED);
        m_isLoading.store(false);
    });
    
    std::lock_guard<std::mutex> lock(m_resultMutex);
    m_futures.push_back(std::move(future));
}

void AsyncLoader::Update() {
    m_animTimer += GetFrameTime();
    
    if (m_isLoading.load()) {
        m_dotsCount = static_cast<int>(m_animTimer * 3.0f) % 4;
    }
    
    // 清理已完成的 futures
    std::lock_guard<std::mutex> lock(m_resultMutex);
    m_futures.erase(
        std::remove_if(m_futures.begin(), m_futures.end(),
            [](std::future<void>& f) {
                if (!f.valid()) return true;
                return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            }),
        m_futures.end()
    );
}

void AsyncLoader::DrawLoadingScreen(int screenWidth, int screenHeight) {
    if (!m_isLoading.load() && m_loadState.load() != LoadState::COMPLETED) return;
    
    // 半透明背景
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.75f));
    
    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;
    
    // 旋转圆圈动画
    float rotation = m_animTimer * 360.0f;
    float radius = 35.0f;
    
    for (int i = 0; i < 12; i++) {
        float angle = static_cast<float>(i) * 30.0f + rotation;
        float alpha = 0.2f + (static_cast<float>(i) / 12.0f) * 0.8f;
        
        float x = centerX + cosf(angle * DEG2RAD) * radius;
        float y = centerY - 50 + sinf(angle * DEG2RAD) * radius;
        
        DrawCircle(static_cast<int>(x), static_cast<int>(y), 3, Fade(SKYBLUE, alpha));
    }
    
    // 标题文字
    const char* title = "Loading";
    int titleSize = 36;
    int titleWidth = MeasureText(title, titleSize);
    DrawText(title, static_cast<int>(centerX - titleWidth / 2), 
             static_cast<int>(centerY - 100), titleSize, WHITE);
    
    // 动态点
    std::string dots;
    for (int i = 0; i < m_dotsCount; i++) dots += ".";
    DrawText(dots.c_str(), static_cast<int>(centerX + titleWidth / 2), 
             static_cast<int>(centerY - 100), titleSize, Fade(WHITE, 0.5f));
    
    // 消息文字
    std::string message;
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        message = m_loadMessage;
    }
    if (!message.empty()) {
        int msgWidth = MeasureText(message.c_str(), 16);
        DrawText(message.c_str(), static_cast<int>(centerX - msgWidth / 2), 
                 static_cast<int>(centerY + 20), 16, LIGHTGRAY);
    }
    
    // 进度条
    float barWidth = 300.0f;
    float barHeight = 20.0f;
    float barX = centerX - barWidth / 2;
    float barY = centerY + 50;
    
    // 背景
    DrawRectangleRounded({barX, barY, barWidth, barHeight}, 0.5f, 8, Color{40, 40, 50, 255});
    
    // 填充
    float progress = m_progress.load();
    if (progress > 0.0f) {
        Color progressColor = (m_loadState.load() == LoadState::COMPLETED) ? GREEN : SKYBLUE;
        DrawRectangleRounded(
            {barX, barY, barWidth * progress, barHeight}, 
            0.5f, 8, progressColor
        );
    }
    
    // 百分比
    int percent = static_cast<int>(progress * 100);
    DrawText(TextFormat("%d%%", percent), 
             static_cast<int>(centerX - 15), static_cast<int>(barY + 2), 16, WHITE);
    
    // 完成提示
    if (m_loadState.load() == LoadState::COMPLETED) {
        DrawText("Complete! Press any key...", 
                 static_cast<int>(centerX - 80), static_cast<int>(barY + 30), 16, Fade(GREEN, 0.8f));
    }
}

void AsyncLoader::Reset() {
    m_isLoading = false;
    m_loadState = LoadState::IDLE;
    m_progress = 0.0f;
    m_completedTasks = 0;
    m_totalTasks = 0;
    m_loadMessage.clear();
}
