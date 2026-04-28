# 多线程安全指南 - Breakout Async Loading

## 如何避免数据竞争

### 1. 互斥锁保护共享数据
```cpp
std::mutex colorMutex;
std::vector<Color> sharedColors;

// 工作线程写入
{
    std::lock_guard<std::mutex> lock(colorMutex);
    sharedColors = newColors;  // 安全写入
}

// 主线程读取
{
    std::lock_guard<std::mutex> lock(colorMutex);
    for (auto& brick : bricks) {
        brick.color = sharedColors[i];  // 安全读取
    }
}
