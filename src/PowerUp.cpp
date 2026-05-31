#include "PowerUp.h"
#include "Ball.h"
#include "Paddle.h"
#include "GameConstants.h"
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI);

SpeedBoostEffect::SpeedBoostEffect() : originalSpeed(1.0f), applied(false) {}
void SpeedBoostEffect::Apply(GameState* state) {
    if (!applied && state->gameSpeed) {
        originalSpeed = *state->gameSpeed;
        *state->gameSpeed = originalSpeed * 1.5f;
        applied = true;
    }
}
void SpeedBoostEffect::Remove(GameState* state) {
    if (applied && state->gameSpeed) {
        *state->gameSpeed = originalSpeed;
        applied = false;
    }
}

SlowMotionEffect::SlowMotionEffect() : originalSpeed(1.0f), applied(false) {}
void SlowMotionEffect::Apply(GameState* state) {
    if (!applied && state->gameSpeed) {
        originalSpeed = *state->gameSpeed;
        *state->gameSpeed = originalSpeed * 0.6f;
        applied = true;
    }
}
void SlowMotionEffect::Remove(GameState* state) {
    if (applied && state->gameSpeed) {
        *state->gameSpeed = originalSpeed;
        applied = false;
    }
}

ExpandPaddleEffect::ExpandPaddleEffect() : originalWidth(120.0f), applied(false) {}
void ExpandPaddleEffect::Apply(GameState* state) {
    if (!applied && state->paddle) {
        originalWidth = state->paddle->GetRect().width;
        state->paddle->SetWidth(originalWidth * 1.6f);
        applied = true;
    }
}
void ExpandPaddleEffect::Remove(GameState* state) {
    if (applied && state->paddle) {
        state->paddle->ResetWidth();
        applied = false;
    }
}

void ExtraLifeEffect::Apply(GameState* state) {
    if (state->lives) (*state->lives)++;
}

MultiBallEffect::MultiBallEffect(int count) : ballCount(count) {}
void MultiBallEffect::Apply(GameState* state) {
    if (state->balls->empty()) return;
    
    Ball& originalBall = (*state->balls)[0];
    Vector2 originalPos = originalBall.GetPosition();
    float radius = originalBall.GetRadius();
    float speedMag = 7.0f;
    
    for (int i = 0; i < ballCount; i++) {
        float angle = angleDist(gen);
        Vector2 newSpeed;
        newSpeed.x = cosf(angle) * speedMag;
        newSpeed.y = -fabsf(sinf(angle)) * speedMag;
        state->balls->emplace_back(originalPos, newSpeed, radius);
    }
}

PowerUp::PowerUp(Vector2 pos, std::unique_ptr<PowerUpEffect> eff)
    : position(pos), velocity{0.0f, 2.5f}, effect(std::move(eff)), active(true), lifetime(10.0f), glowTimer(0.0f) {
    rect = {pos.x - 18, pos.y - 18, 36, 36};
}

void PowerUp::Update(float deltaTime) {
    position.y += velocity.y * deltaTime * 60.0f;
    rect.x = position.x - rect.width / 2;
    rect.y = position.y - rect.height / 2;
    lifetime -= deltaTime;
    glowTimer += deltaTime;
    
    if (position.y > SCREEN_HEIGHT + 100 || lifetime <= 0) {
        active = false;
    }
}

void PowerUp::Draw() {
    if (!active) return;
    
    Color col = effect->GetColor();
    float glowSize = 20.0f + sinf(glowTimer * 5.0f) * 4.0f;
    
    DrawCircleV(position, glowSize, Fade(col, 0.3f));
    DrawRectangleRounded(rect, 0.3f, 8, col);
    DrawRectangleRoundedLines(rect, 0.3f, 8, 2, WHITE);
    
    const char* text = effect->GetName().c_str();
    int textWidth = MeasureText(text, 14);
    DrawText(text, position.x - textWidth/2, position.y - 7, 14, WHITE);
}

bool PowerUp::CheckCollision(Rectangle paddleRect) {
    return CheckCollisionRecs(rect, paddleRect);
}

void PowerUp::Apply(GameState* state) {
    if (effect) effect->Apply(state);
    active = false;
}

std::unique_ptr<PowerUp> PowerUpFactory::CreateRandomPowerUp(Vector2 position) {
    static std::uniform_int_distribution<int> typeDist(0, 4);
    int type = typeDist(gen);
    
    switch(type) {
        case 0: return std::make_unique<PowerUp>(position, std::make_unique<SpeedBoostEffect>());
        case 1: return std::make_unique<PowerUp>(position, std::make_unique<SlowMotionEffect>());
        case 2: return std::make_unique<PowerUp>(position, std::make_unique<ExpandPaddleEffect>());
        case 3: return std::make_unique<PowerUp>(position, std::make_unique<ExtraLifeEffect>());
        default: return std::make_unique<PowerUp>(position, std::make_unique<MultiBallEffect>(2));
    }
}
