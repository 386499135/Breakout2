#ifndef PADDLE_H
#define PADDLE_H

#include "raylib.h"
#include "GameConstants.h"

class Paddle {
private:
    Rectangle rect;
    float originalWidth;
    Color color;
    
public:
    Paddle(float x = 340, float y = 550, float width = 120, float height = 15);
    
    void MoveLeft(float speed);
    void MoveRight(float speed);
    void Draw();
    
    void SetWidth(float width);
    void ResetWidth();
    void SetColor(Color c) { color = c; }
    
    Rectangle GetRect() const { return rect; }
    float GetOriginalWidth() const { return originalWidth; }
};

#endif
