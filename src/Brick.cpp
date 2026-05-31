#include "Brick.h"

Brick::Brick() : rect{0,0,0,0}, active(false), color(WHITE), health(1) {}

Brick::Brick(float x, float y, float w, float h, Color c, int hp) {
    rect = {x, y, w, h};
    active = true;
    color = c;
    health = hp;
}

void Brick::Draw() {
    if (!active) return;
    
    Color drawColor = color;
    if (health == 2) {
        drawColor.r = color.r * 0.7f;
        drawColor.g = color.g * 0.7f;
        drawColor.b = color.b * 0.7f;
    } else if (health >= 3) {
        drawColor.r = color.r * 0.5f;
        drawColor.g = color.g * 0.5f;
        drawColor.b = color.b * 0.5f;
    }
    
    DrawRectangleRec(rect, drawColor);
    DrawRectangleLinesEx(rect, 1, Fade(WHITE, 0.5f));
    
    if (health > 1) {
        DrawText(TextFormat("%d", health), rect.x + rect.width/2 - 5, rect.y + rect.height/2 - 6, 12, WHITE);
    }
}

bool Brick::Hit() {
    health--;
    if (health <= 0) {
        active = false;
        return true;
    }
    return false;
}
