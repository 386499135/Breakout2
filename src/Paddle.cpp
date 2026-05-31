#include "Paddle.h"
#include "GameConstants.h"

Paddle::Paddle(float x, float y, float width, float height) {
    rect = {x, y, width, height};
    originalWidth = width;
    color = BLUE;
}

void Paddle::MoveLeft(float speed) {
    rect.x -= speed;
    if (rect.x < 5) rect.x = 5;
}

void Paddle::MoveRight(float speed) {
    rect.x += speed;
    if (rect.x + rect.width > SCREEN_WIDTH - 5) rect.x = SCREEN_WIDTH - rect.width - 5;
}

void Paddle::Draw() {
    DrawRectangleRounded({rect.x - 2, rect.y - 2, rect.width + 4, rect.height + 4}, 0.3f, 8, Fade(SKYBLUE, 0.3f));
    DrawRectangleRounded(rect, 0.3f, 8, color);
    DrawRectangleRoundedLines(rect, 0.3f, 8, 2, SKYBLUE);
    
    if (rect.width > originalWidth + 5) {
        DrawText("WIDE!", rect.x + rect.width/2 - 20, rect.y - 20, 16, YELLOW);
    }
}

void Paddle::SetWidth(float width) {
    float oldWidth = rect.width;
    rect.width = width;
    rect.x = rect.x + (oldWidth - width) / 2;
    if (rect.x < 5) rect.x = 5;
    if (rect.x + rect.width > SCREEN_WIDTH - 5) rect.x = SCREEN_WIDTH - rect.width - 5;
}

void Paddle::ResetWidth() {
    SetWidth(originalWidth);
}
