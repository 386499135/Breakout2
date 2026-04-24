#include "PowerUp.h"
#include "Ball.h"
#include "Paddle.h"
#include <cmath>
#include <random>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 全局随机数生成器
static std::random_device rd;
static std::mt19937 gen(rd());

// ============== ParticleSystem 实现 ==============
void ParticleSystem::EmitBrickBreak(Vector2 position, Color brickColor, int count) {
    std::uniform_real_distribution<float> velDist(-3.0f, 3.0f);
    std::uniform_real_distribution<float> sizeDist(2.0f, 5.0f);
    std::uniform_real_distribution<float> lifeDist(0.5f, 1.5f);
    
    for (int i = 0; i < count; i++) {
        Particle p;
        p.position = position;
        p.velocity = {velDist(gen), velDist(gen)};
        p.color = brickColor;
        p.life = lifeDist(gen);
        p.size = sizeDist(gen);
        p.active = true;
        particles.push_back(p);
    }
}

void ParticleSystem::EmitGlow(Vector2 position, Color color, float radius) {
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * (float)M_PI);
    std::uniform_real_distribution<float> sizeDist(1.0f, 3.0f);
    
    for (int i = 0; i < 8; i++) {
        Particle p;
        float angle = angleDist(gen);
        float dist = radius * 0.5f;
        p.position.x = position.x + cosf(angle) * dist;
        p.position.y = position.y + sinf(angle) * dist;
        p.velocity.x = cosf(angle) * 0.5f;
        p.velocity.y = sinf(angle) * 0.5f - 1.0f;
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
        p.velocity.y += 0.2f;
        p.life -= deltaTime;
        p.color.a = (unsigned char)(255 * (p.life / 1.5f));
        
        if (p.life <= 0) {
            p.active = false;
        }
    }
    
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return !p.active; }), particles.end());
}

void ParticleSystem::Draw() {
    for (const auto& p : particles) {
        if (p.active) {
            DrawCircleV(p.position, p.size, p.color);
        }
    }
}

// ============== 道具效果实现 ==============
void SpeedBoostEffect::Apply(GameState* state) {
    if (!applied) {
        originalSpeed = *state->gameSpeed;
        *state->gameSpeed = originalSpeed * multiplier;
        applied = true;
    }
}

void SpeedBoostEffect::Remove(GameState* state) {
    if (applied) {
        *state->gameSpeed = originalSpeed;
    }
}

void SlowMotionEffect::Apply(GameState* state) {
    if (!applied) {
        originalSpeed = *state->gameSpeed;
        *state->gameSpeed = originalSpeed * multiplier;
        applied = true;
    }
}

void SlowMotionEffect::Remove(GameState* state) {
    if (applied) {
        *state->gameSpeed = originalSpeed;
    }
}

void MultiBallEffect::Apply(GameState* state) {
    if (state->balls->empty()) return;
    
    Ball& originalBall = (*state->balls)[0];
    Vector2 originalPos = originalBall.GetPosition();
    Vector2 originalSpeed = originalBall.GetSpeed();
    float radius = originalBall.GetRadius();
    
    for (int i = 0; i < ballCount; i++) {
        float angle = (i * 360.0f / ballCount) * ((float)M_PI / 180.0f);
        Vector2 newSpeed;
        newSpeed.x = originalSpeed.x + cosf(angle) * 3.0f;
        newSpeed.y = originalSpeed.y + sinf(angle) * 3.0f;
        state->balls->emplace_back(originalPos, newSpeed, radius);
    }
}

void ExpandPaddleEffect::Apply(GameState* state) {
    if (!applied) {
        originalWidth = state->paddle->GetRect().width;
        Rectangle rect = state->paddle->GetRect();
        float newWidth = originalWidth * multiplier;
        
        // 限制最大宽度为 200 像素
        if (newWidth > 200.0f) {
            newWidth = 200.0f;
        }
        
        rect.width = newWidth;
        state->paddle->SetRect(rect);
        applied = true;
        timer = 0.0f;
        duration = 5.0f;
    }
}

void ExpandPaddleEffect::Remove(GameState* state) {
    if (applied) {
        Rectangle rect = state->paddle->GetRect();
        rect.width = originalWidth;
        state->paddle->SetRect(rect);
    }
}

void ExpandPaddleEffect::Update(GameState* state, float deltaTime) {
    if (applied) {
        timer += deltaTime;
        if (timer >= duration) {
            Remove(state);
            applied = false;
        }
    }
}

void ExtraLifeEffect::Apply(GameState* state) {
    (*state->lives)++;
}

// ============== PowerUp 实现 ==============
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
    
    float glowSize = 15.0f + sinf(glowTimer * 5.0f) * 3.0f;
    Color glowColor = color;
    glowColor.a = 100;
    DrawCircleV(position, glowSize, glowColor);
    
    DrawRectangleRec(rect, color);
    DrawRectangleLinesEx(rect, 2, WHITE);
    
    const char* icon = name.c_str();
    int fontSize = 20;
    int textWidth = MeasureText(icon, fontSize);
    DrawText(icon, (int)(position.x - textWidth/2), (int)(position.y - fontSize/2), fontSize, WHITE);
    
    if (glowTimer > 0.1f) {
        particleSystem.EmitGlow(position, color, glowSize);
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
PowerUpFactory::PowerUpFactory() {
    std::ifstream file("powerups.json");
    if (file.is_open()) {
        try {
            config = json::parse(file);
        } catch (...) {
            config = json::object();
        }
    }
}

std::unique_ptr<PowerUp> PowerUpFactory::CreateRandomPowerUp(Vector2 position) {
    std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
    
    if (!config.contains("powerups")) return nullptr;
    
    auto& powerups = config["powerups"];
    for (auto& [type, data] : powerups.items()) {
        float dropChance = data.value("drop_chance", 0.2f);
        if (chanceDist(gen) < dropChance) {
            return CreatePowerUp(type, position);
        }
    }
    
    return nullptr;
}

std::unique_ptr<PowerUp> PowerUpFactory::CreatePowerUp(const std::string& type, Vector2 position) {
    if (!config.contains("powerups") || !config["powerups"].contains(type)) {
        return nullptr;
    }
    
    auto& data = config["powerups"][type];
    std::string name = data.value("name", "PowerUp");
    auto colorArray = data.value("color", std::vector<int>{255, 255, 255, 255});
    Color color = {
        (unsigned char)colorArray[0],
        (unsigned char)colorArray[1],
        (unsigned char)colorArray[2],
        (unsigned char)colorArray[3]
    };
    
    std::unique_ptr<PowerUpEffect> effect;
    float effectValue = data.value("effect_value", 1.0f);
    
    if (type == "speed_boost") {
        effect = std::make_unique<SpeedBoostEffect>(effectValue);
    } else if (type == "slow_motion") {
        effect = std::make_unique<SlowMotionEffect>(effectValue);
    } else if (type == "multi_ball") {
        effect = std::make_unique<MultiBallEffect>((int)effectValue);
    } else if (type == "expand_paddle") {
        effect = std::make_unique<ExpandPaddleEffect>(effectValue);
    } else if (type == "extra_life") {
        effect = std::make_unique<ExtraLifeEffect>();
    }
    
    if (effect) {
        return std::make_unique<PowerUp>(position, std::move(effect), color, name);
    }
    
    return nullptr;
}

float PowerUpFactory::GetDropChance() const {
    return config.value("global_settings", json::object()).value("base_drop_chance", 0.3f);
}
