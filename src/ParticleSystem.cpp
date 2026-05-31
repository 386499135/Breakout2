#include "ParticleSystem.h"
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void ParticleSystem::EmitBrickBreak(Vector2 pos, Color color, int count) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.position = pos;
        p.velocity.x = (float)((rand() % 100 - 50) / 8.0f);
        p.velocity.y = (float)((rand() % 100 - 50) / 8.0f) - 2.0f;
        p.color = color;
        p.life = 0.8f + (rand() % 10) / 20.0f;
        p.size = 2.0f + (rand() % 4);
        p.active = true;
        particles.push_back(p);
    }
}

void ParticleSystem::EmitPowerUpGlow(Vector2 pos, Color color) {
    for (int i = 0; i < 8; i++) {
        Particle p;
        float angle = (rand() % 360) * M_PI / 180.0f;
        p.position = pos;
        p.velocity.x = cosf(angle) * 2.0f;
        p.velocity.y = sinf(angle) * 2.0f - 1.5f;
        p.color = color;
        p.life = 0.8f;
        p.size = 2.0f;
        p.active = true;
        particles.push_back(p);
    }
}

void ParticleSystem::Update(float deltaTime) {
    for (auto it = particles.begin(); it != particles.end();) {
        it->position.x += it->velocity.x;
        it->position.y += it->velocity.y;
        it->velocity.y += 0.2f;
        it->life -= deltaTime;
        if (it->life <= 0) {
            it = particles.erase(it);
        } else {
            ++it;
        }
    }
}

void ParticleSystem::Draw() {
    for (const auto& p : particles) {
        DrawCircleV(p.position, p.size, Fade(p.color, p.life));
    }
}

void ParticleSystem::Clear() {
    particles.clear();
}
