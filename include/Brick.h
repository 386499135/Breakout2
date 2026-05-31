#ifndef BRICK_H
#define BRICK_H

#include "raylib.h"

class Brick {
private:
    Rectangle rect;
    bool active;
    Color color;
    int health;
    
public:
    Brick();
    Brick(float x, float y, float w, float h, Color c, int hp = 1);
    
    void Draw();
    bool Hit();
    
    bool IsActive() const { return active; }
    void SetActive(bool a) { active = a; }
    Rectangle GetRect() const { return rect; }
    Color GetColor() const { return color; }
    int GetHealth() const { return health; }
};

#endif
