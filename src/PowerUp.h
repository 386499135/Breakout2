#ifndef POWERUP_H
#define POWERUP_H
#include "raylib.h"
#include <string>
#include <memory>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

// 前置声明
class Ball;
class Paddle;

// GameState 类
class GameState {
public:
    std::vector<Ball>* balls;
    Paddle* paddle;
    int* lives;
    int* score;
    float* gameSpeed;
    
    GameState(std::vector<Ball>* b, Paddle* p, int* l, int* s, float* gs)
        : balls(b), paddle(p), lives(l), score(s), gameSpeed(gs) {}
};

// 粒子结构
struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
    float size;
    bool active;
};

// 粒子系统
class ParticleSystem {
private:
    std::vector<Particle> particles;
    
public:
    ParticleSystem() = default;
    
    void EmitBrickBreak(Vector2 position, Color brickColor, int count = 15);
    void EmitPowerUpGlow(Vector2 position, Color color);
    void Update();
    void Draw();
    void Clear() { particles.clear(); }
};

// 道具效果基类
class PowerUpEffect {
public:
    virtual ~PowerUpEffect() = default;
    virtual void Apply(GameState* state) = 0;
    virtual void Remove(GameState* state) = 0;
    virtual bool IsTimed() const { return false; }
    virtual float GetDuration() const { return 0.0f; }
    virtual std::string GetName() const = 0;
    virtual Color GetColor() const = 0;
};

// 速度提升效果
class SpeedBoostEffect : public PowerUpEffect {
private:
    float multiplier;
    float originalSpeed;
    bool applied;
    
public:
    SpeedBoostEffect(float mult = 1.5f) : multiplier(mult), originalSpeed(1.0f), applied(false) {}
    void Apply(GameState* state) override;
    void Remove(GameState* state) override;
    bool IsTimed() const override { return true; }
    float GetDuration() const override { return 8.0f; }
    std::string GetName() const override { return "SPEED+"; }
    Color GetColor() const override { return GREEN; }
};

// 减速效果
class SlowMotionEffect : public PowerUpEffect {
private:
    float multiplier;
    float originalSpeed;
    bool applied;
    
public:
    SlowMotionEffect(float mult = 0.5f) : multiplier(mult), originalSpeed(1.0f), applied(false) {}
    void Apply(GameState* state) override;
    void Remove(GameState* state) override;
    bool IsTimed() const override { return true; }
    float GetDuration() const override { return 6.0f; }
    std::string GetName() const override { return "SLOW"; }
    Color GetColor() const override { return BLUE; }
};

// 多重球效果
class MultiBallEffect : public PowerUpEffect {
private:
    int ballCount;
    
public:
    MultiBallEffect(int count = 2) : ballCount(count) {}
    void Apply(GameState* state) override;
    void Remove(GameState* state) override {}
    bool IsTimed() const override { return false; }
    std::string GetName() const override { return "MULTI"; }
    Color GetColor() const override { return PURPLE; }
};

// 加长挡板效果
class ExpandPaddleEffect : public PowerUpEffect {
private:
    float multiplier;
    float originalWidth;
    bool applied;
    
public:
    ExpandPaddleEffect(float mult = 1.5f) : multiplier(mult), originalWidth(0.0f), applied(false) {}
    void Apply(GameState* state) override;
    void Remove(GameState* state) override;
    bool IsTimed() const override { return true; }
    float GetDuration() const override { return 10.0f; }
    std::string GetName() const override { return "WIDE"; }
    Color GetColor() const override { return YELLOW; }
};

// 额外生命效果
class ExtraLifeEffect : public PowerUpEffect {
public:
    void Apply(GameState* state) override;
    void Remove(GameState* state) override {}
    bool IsTimed() const override { return false; }
    std::string GetName() const override { return "LIFE+"; }
    Color GetColor() const override { return RED; }
};

// 道具类
class PowerUp {
private:
    Vector2 position;
    Vector2 velocity;
    Rectangle rect;
    std::unique_ptr<PowerUpEffect> effect;
    bool active;
    float lifetime;
    float glowTimer;
    
public:
    PowerUp(Vector2 pos, std::unique_ptr<PowerUpEffect> eff);
    
    void Update(float deltaTime);
    void Draw(ParticleSystem& particleSystem);
    bool CheckCollision(Rectangle paddleRect);
    void Apply(GameState* state);
    
    bool IsActive() const { return active && lifetime > 0; }
    Vector2 GetPosition() const { return position; }
    std::string GetName() const { return effect ? effect->GetName() : "?"; }
    Color GetColor() const { return effect ? effect->GetColor() : WHITE; }
};

// 道具工厂
class PowerUpFactory {
public:
    PowerUpFactory() = default;
    std::unique_ptr<PowerUp> CreateRandomPowerUp(Vector2 position);
    float GetDropChance() const { return 0.3f; }
};

#endif
