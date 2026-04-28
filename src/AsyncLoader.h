#ifndef ASYNC_LOADER_H
#define ASYNC_LOADER_H

#include "raylib.h"
#include <string>
#include <future>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>

// 加载状态枚举
enum class LoadState {
    IDLE,
    LOADING,
    COMPLETED,
    FAILED
};

// 线程安全的纹理缓存
class TextureCache {
private:
    std::unordered_map<std::string, Texture2D> cache;
    mutable std::mutex cacheMutex;
    
public:
    ~TextureCache() { Clear(); }
    
    Texture2D GetOrLoad(const std::string& path);
    bool Preload(const std::string& path);
    bool Has(const std::string& path) const;
    Texture2D Get(const std::string& path) const;
    void Clear();
    size_t Size() const;
};

// 异步加载管理器
class AsyncLoader {
private:
    std::atomic<bool> m_isLoading{false};
    std::atomic<LoadState> m_loadState{LoadState::IDLE};
    std::mutex m_stateMutex;
    std::mutex m_resultMutex;
    
    std::string m_loadMessage;
    std::atomic<float> m_progress{0.0f};
    TextureCache m_textureCache;
    
    int m_nextTaskId{0};
    std::atomic<int> m_completedTasks{0};
    std::atomic<int> m_totalTasks{0};
    
    // 动画相关
    float m_animTimer{0.0f};
    int m_dotsCount{0};
    
    // 存储 future 以保持任务存活
    std::vector<std::future<void>> m_futures;
    
public:
    AsyncLoader() = default;
    ~AsyncLoader() = default;
    
    // 禁止拷贝
    AsyncLoader(const AsyncLoader&) = delete;
    AsyncLoader& operator=(const AsyncLoader&) = delete;
    
    // 启动模拟重负载任务（按键 L 触发）
    void StartSimulatedLoad(float duration = 2.0f);
    
    // 启动纹理加载任务
    void StartTextureLoad(const std::string& path);
    
    // 更新（每帧调用）
    void Update();
    
    // 绘制加载界面
    void DrawLoadingScreen(int screenWidth, int screenHeight);
    
    // 状态查询
    bool IsLoading() const { return m_isLoading.load(); }
    LoadState GetState() const { return m_loadState.load(); }
    float GetProgress() const { return m_progress.load(); }
    
    // 获取纹理缓存
    TextureCache& GetTextureCache() { return m_textureCache; }
    
    // 重置加载器
    void Reset();
};

#endif // ASYNC_LOADER_H
