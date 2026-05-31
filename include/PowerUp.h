#ifndef POWERUP_H
#define POWERUP_H

#include "raylib.h"
#include <string>
#include <memory>
#include <vector>
#include <random>

class Ball;
class Paddle;

struct GameState {
    std::vector<Ball>* balls;
    Paddle* paddle;
    int* lives;
    int* score;
    float* gameSpeed;
    
    GameState(std::vector<Ball>* b, Paddle* p, int* l, int* s, float* gs)
        : balls(b), paddle(p), lives(l), score(s), gameSpeed(gs) {}
};

class PowerUpEffect {
public:
    virtual ~PowerUpEffect() = default;
    virtual void Apply(GameState* state) = 0;
    virtual void Remove(GameState* state) = 0;
    virtual std::string GetName() const = 0;
    virtual Color GetColor() const = 0;
    virtual bool IsTimed() const { return true; }
    virtual float GetDuration() const { return 8.0f; }
};

class SpeedBoostEffect : public PowerUpEffect {
    float originalSpeed;
    bool applied;
public:
    SpeedBoostEffect();
    void Apply(GameState* state) override;
    void Remove(GameState* state) override;
    std::string GetName() const override { return "SPEED+"; }
    Color GetColor() const override { return GREEN; }
};

class SlowMotionEffect : public PowerUpEffect {
    float originalSpeed;
    bool applied;
public:
    SlowMotionEffect();
    void Apply(GameState* state) override;
    void Remove(GameState* state) override;
    std::string GetName() const override { return "SLOW"; }
    Color GetColor() const override { return SKYBLUE; }
};

class ExpandPaddleEffect : public PowerUpEffect {
    float originalWidth;
    bool applied;
public:
    ExpandPaddleEffect();
    void Apply(GameState* state) override;
    void Remove(GameState* state) override;
    std::string GetName() const override { return "WIDE"; }
    Color GetColor() const override { return YELLOW; }
};

class ExtraLifeEffect : public PowerUpEffect {
public:
    void Apply(GameState* state) override;
    void Remove(GameState* state) override {}
    bool IsTimed() const override { return false; }
    std::string GetName() const override { return "LIFE+"; }
    Color GetColor() const override { return RED; }
};

class MultiBallEffect : public PowerUpEffect {
    int ballCount;
public:
    MultiBallEffect(int count = 2);
    void Apply(GameState* state) override;
    void Remove(GameState* state) override {}
    bool IsTimed() const override { return false; }
    std::string GetName() const override { return "MULTI"; }
    Color GetColor() const override { return PURPLE; }
};

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
    void Draw();
    bool CheckCollision(Rectangle paddleRect);
    void Apply(GameState* state);
    bool IsActive() const { return active && lifetime > 0; }
    PowerUpEffect* GetEffect() const { return effect.get(); }
};

class PowerUpFactory {
public:
    std::unique_ptr<PowerUp> CreateRandomPowerUp(Vector2 position);
    float GetDropChance() const { return 0.25f; }
};

#endif
