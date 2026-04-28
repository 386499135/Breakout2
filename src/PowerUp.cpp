#include "PowerUp.h"
#include "Ball.h"
#include "Paddle.h"
#include <cmath>
#include <random>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 全局随机数生成器
static std::random_device rd;
static std::mt19937 gen(rd());

// ============== ParticleSystem 实现 ==============
void ParticleSystem::EmitBrickBreak(Vector2 position, Color brickColor, int count) {
    std::uniform_real_distribution<float> velDist(-4.0f, 4.0f);
    std::uniform_real_distribution<float> sizeDist(2.0f, 6.0f);
    std::uniform_real_distribution<float> lifeDist(0.5f, 1.5f);
    
    for (int i = 0; i < count; i++) {
        Particle p;
        p.position = position;
        p.velocity = {velDist(gen), velDist(gen) - 2.0f};
        p.color = brickColor;
        p.life = lifeDist(gen);
        p.size = sizeDist(gen);
        p.active = true;
        particles.push_back(p);
    }
}

void ParticleSystem::EmitPowerUpGlow(Vector2 position, Color color) {
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * (float)M_PI);
    std::uniform_real_distribution<float> sizeDist(1.0f, 3.0f);
    
    for (int i = 0; i < 6; i++) {
        Particle p;
        float angle = angleDist(gen);
        p.position.x = position.x + cosf(angle) * 10.0f;
        p.position.y = position.y + sinf(angle) * 10.0f;
        p.velocity.x = cosf(angle) * 1.5f;
        p.velocity.y = sinf(angle) * 1.5f - 2.0f;
        p.color = color;
        p.color.a = 150;
        p.life = 0.8f;
        p.size = sizeDist(gen);
        p.active = true;
        particles.push_back(p);
    }
}

void ParticleSystem::Update() {
    float deltaTime = GetFrameTime();
    for (auto& p : particles) {
        if (!p.active) continue;
        
        p.position.x += p.velocity.x;
        p.position.y += p.velocity.y;
        p.velocity.y += 0.3f;  // 重力
        p.life -= deltaTime;
        p.color.a = (unsigned char)(255 * std::max(0.0f, p.life / 1.5f));
        
        if (p.life <= 0) {
            p.active = false;
        }
    }
    
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return !p.active; }), particles.end());
}

void ParticleSystem::Draw() {
    for (const auto& p : particles) {
        if (p.active && p.life > 0) {
            DrawCircleV(p.position, p.size, p.color);
        }
    }
}

// ============== 道具效果实现 ==============
void SpeedBoostEffect::Apply(GameState* state) {
    if (!applied && state->gameSpeed) {
        originalSpeed = *state->gameSpeed;
        *state->gameSpeed = originalSpeed * multiplier;
        applied = true;
    }
}

void SpeedBoostEffect::Remove(GameState* state) {
    if (applied && state->gameSpeed) {
        *state->gameSpeed = originalSpeed;
        applied = false;
    }
}

void SlowMotionEffect::Apply(GameState* state) {
    if (!applied && state->gameSpeed) {
        originalSpeed = *state->gameSpeed;
        *state->gameSpeed = originalSpeed * multiplier;
        applied = true;
    }
}

void SlowMotionEffect::Remove(GameState* state) {
    if (applied && state->gameSpeed) {
        *state->gameSpeed = originalSpeed;
        applied = false;
    }
}

void MultiBallEffect::Apply(GameState* state) {
    if (state->balls->empty()) return;
    
    Ball& originalBall = (*state->balls)[0];
    Vector2 originalPos = originalBall.GetPosition();
    float radius = originalBall.GetRadius();
    
    for (int i = 0; i < ballCount; i++) {
        float angle = (float)i * 360.0f / ballCount * ((float)M_PI / 180.0f);
        Vector2 newSpeed;
        newSpeed.x = cosf(angle) * 5.0f;
        newSpeed.y = -5.0f;
        state->balls->emplace_back(originalPos, newSpeed, radius);
    }
}

void ExpandPaddleEffect::Apply(GameState* state) {
    if (!applied && state->paddle) {
        originalWidth = state->paddle->GetRect().width;
        Rectangle rect = state->paddle->GetRect();
        float newWidth = originalWidth * multiplier;
        if (newWidth > 250.0f) newWidth = 250.0f;
        rect.width = newWidth;
        // 保持挡板在屏幕内
        if (rect.x + rect.width > 795) rect.x = 795 - rect.width;
        state->paddle->SetRect(rect);
        applied = true;
    }
}

void ExpandPaddleEffect::Remove(GameState* state) {
    if (applied && state->paddle) {
        Rectangle rect = state->paddle->GetRect();
        rect.width = originalWidth;
        if (rect.x + rect.width > 795) rect.x = 795 - rect.width;
        state->paddle->SetRect(rect);
        applied = false;
    }
}

void ExtraLifeEffect::Apply(GameState* state) {
    if (state->lives) {
        (*state->lives)++;
    }
}

// ============== PowerUp 实现 ==============
PowerUp::PowerUp(Vector2 pos, std::unique_ptr<PowerUpEffect> eff)
    : position(pos)
    , velocity({0.0f, 2.5f})
    , effect(std::move(eff))
    , active(true)
    , lifetime(10.0f)
    , glowTimer(0.0f) {
    rect = {pos.x - 18, pos.y - 18, 36, 36};
}

void PowerUp::Update(float deltaTime) {
    position.y += velocity.y * deltaTime * 60.0f;
    rect.x = position.x - rect.width / 2;
    rect.y = position.y - rect.height / 2;
    lifetime -= deltaTime;
    glowTimer += deltaTime;
    
    if (position.y > 650) {
        active = false;
    }
}

void PowerUp::Draw(ParticleSystem& particleSystem) {
    if (!active) return;
    
    Color col = GetColor();
    
    // 光晕效果
    float glowSize = 20.0f + sinf(glowTimer * 5.0f) * 5.0f;
    Color glowColor = col;
    glowColor.a = 80;
    DrawCircleV(position, glowSize, glowColor);
    
    // 主体
    DrawRectangleRounded(rect, 0.3f, 8, col);
    DrawRectangleRoundedLines(rect, 0.3f, 8, 2, WHITE);
    
    // 名称
    const char* icon = GetName().c_str();
    int fontSize = 16;
    int textWidth = MeasureText(icon, fontSize);
    DrawText(icon, (int)(position.x - textWidth/2), (int)(position.y - fontSize/2), fontSize, WHITE);
    
    // 粒子效果
    if (glowTimer > 0.15f) {
        particleSystem.EmitPowerUpGlow(position, col);
        glowTimer = 0.0f;
    }
}

bool PowerUp::CheckCollision(Rectangle paddleRect) {
    return CheckCollisionRecs(rect, paddleRect);
}

void PowerUp::Apply(GameState* state) {
    if (effect) {
        effect->Apply(state);
    }
    active = false;
}

// ============== PowerUpFactory 实现 ==============
std::unique_ptr<PowerUp> PowerUpFactory::CreateRandomPowerUp(Vector2 position) {
    std::uniform_int_distribution<int> typeDist(0, 4);
    
    int type = typeDist(gen);
    
    switch (type) {
        case 0:
            return std::make_unique<PowerUp>(position, std::make_unique<SpeedBoostEffect>(1.5f));
        case 1:
            return std::make_unique<PowerUp>(position, std::make_unique<SlowMotionEffect>(0.5f));
        case 2:
            return std::make_unique<PowerUp>(position, std::make_unique<MultiBallEffect>(2));
        case 3:
            return std::make_unique<PowerUp>(position, std::make_unique<ExpandPaddleEffect>(1.6f));
        case 4:
            return std::make_unique<PowerUp>(position, std::make_unique<ExtraLifeEffect>());
        default:
            return nullptr;
    }
}
