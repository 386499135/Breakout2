#ifndef POWERUP_H
#define POWERUP_H
#include "raylib.h"
#include "Paddle.h"  // 需要完整定义
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>
#include <fstream>
#include <random>
#include <cmath>

using json = nlohmann::json;

// 前置声明
class Ball;

// 粒子系统
struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
    float size;
    bool active;
};

class ParticleSystem {
private:
    std::vector<Particle> particles;
    
public:
    ParticleSystem() = default;
    
    void EmitBrickBreak(Vector2 position, Color brickColor, int count = 15);
    void EmitGlow(Vector2 position, Color color, float radius);
    void Update();
    void Draw();
    void Clear() { particles.clear(); }
};

// GameState类定义（移到前面）
class GameState {
public:
    std::vector<Ball>* balls;
    Paddle* paddle;
    int* lives;
    int* score;
    float* gameSpeed;
    float paddleOriginalWidth;
    
    GameState(std::vector<Ball>* b, Paddle* p, int* l, int* s, float* gs)
        : balls(b), paddle(p), lives(l), score(s), gameSpeed(gs) {
        paddleOriginalWidth = paddle->GetRect().width;
    }
};

// 抽象道具效果基类（工厂模式）
class PowerUpEffect {
public:
    virtual ~PowerUpEffect() = default;
    virtual void Apply(GameState* state) = 0;
    virtual void Remove(GameState* state) = 0;
    virtual void Update(GameState* state, float deltaTime) {}
    virtual bool IsActive() const { return true; }
    virtual std::string GetName() const = 0;
};

// 具体道具效果实现
class SpeedBoostEffect : public PowerUpEffect {
private:
    float multiplier;
    float originalSpeed;
    bool applied;
    
public:
    SpeedBoostEffect(float mult) : multiplier(mult), originalSpeed(0), applied(false) {}
    void Apply(GameState* state) override;
    void Remove(GameState* state) override;
    std::string GetName() const override { return "速度提升"; }
};

class SlowMotionEffect : public PowerUpEffect {
private:
    float multiplier;
    float originalSpeed;
    bool applied;
    
public:
    SlowMotionEffect(float mult) : multiplier(mult), originalSpeed(0), applied(false) {}
    void Apply(GameState* state) override;
    void Remove(GameState* state) override;
    std::string GetName() const override { return "减速"; }
};

class MultiBallEffect : public PowerUpEffect {private:
    int ballCount;
    
public:
    MultiBallEffect(int count) : ballCount(count) {}
    void Apply(GameState* state) override;
    void Remove(GameState* state) override {}
    std::string GetName() const override { return "多重球"; }
};

class ExpandPaddleEffect : public PowerUpEffect {
private:
    float multiplier;
    float originalWidth;
    bool applied;
     float duration;      // 持续时间
    float timer;         // 计时器
    
public:
    ExpandPaddleEffect(float mult) : multiplier(mult), originalWidth(0), applied(false), duration(5.0f), timer(0.0f) {}
    void Apply(GameState* state) override;
    void Remove(GameState* state) override;
    void Update(GameState* state, float deltaTime) override;
    bool IsActive() const override { return timer < duration; }
    std::string GetName() const override { return "加长挡板"; }
};

class ExtraLifeEffect : public PowerUpEffect {
public:
    ExtraLifeEffect() = default;
    void Apply(GameState* state) override;
    void Remove(GameState* state) override {}
    std::string GetName() const override { return "额外生命"; }
};

// 道具类
class PowerUp {
private:
    Vector2 position;
    Vector2 velocity;
    Rectangle rect;
    std::unique_ptr<PowerUpEffect> effect;
    Color color;
    std::string name;
    bool active;
    float lifetime;
    float glowTimer;
    
public:
    PowerUp(Vector2 pos, std::unique_ptr<PowerUpEffect> eff, Color col, const std::string& n)
        : position(pos), effect(std::move(eff)), color(col), name(n), active(true), lifetime(10.0f), glowTimer(0.0f) {
        rect = {pos.x - 15, pos.y - 15, 30, 30};
        velocity = {0, 2.5f};
    }
    
    void Update(float deltaTime);
    void Draw(ParticleSystem& particleSystem);
    bool CheckCollision(Rectangle paddleRect);
    void Apply(GameState* state);
    
    bool IsActive() const { return active && lifetime > 0; }
    Vector2 GetPosition() const { return position; }
    std::string GetName() const { return name; }
};

// 道具工厂
class PowerUpFactory {
private:
    json config;
    
public:
    PowerUpFactory();
    std::unique_ptr<PowerUp> CreateRandomPowerUp(Vector2 position);
    std::unique_ptr<PowerUp> CreatePowerUp(const std::string& type, Vector2 position);
    float GetDropChance() const;
};

#endif
