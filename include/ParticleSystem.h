#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "raylib.h"
#include <vector>
#include "GameConstants.h"

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
    void EmitBrickBreak(Vector2 pos, Color color, int count = 15);
    void EmitPowerUpGlow(Vector2 pos, Color color);
    void Update(float deltaTime);
    void Draw();
    void Clear();
    int GetCount() const { return (int)particles.size(); }
};

#endif
