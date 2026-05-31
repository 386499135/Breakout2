#ifndef GAME_CONSTANTS_H
#define GAME_CONSTANTS_H

#include "raylib.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int BRICK_WIDTH = 85;
const int BRICK_HEIGHT = 25;
const float PADDLE_SPEED_NORMAL = 18.0f;
const float PADDLE_SPEED_BOOST = 28.0f;
const float BALL_RADIUS = 10.0f;
const int MAX_PARTICLES = 500;

const Color COLOR_BG = {20, 25, 35, 255};
const Color COLOR_PANEL = {30, 35, 50, 220};
const Color COLOR_BORDER = {60, 65, 85, 255};
const Color COLOR_ACCENT = {100, 150, 255, 255};
const Color COLOR_SUCCESS = {80, 200, 120, 255};
const Color COLOR_WARNING = {255, 180, 50, 255};
const Color COLOR_DANGER = {255, 80, 80, 255};
const Color COLOR_GOLD = {255, 215, 0, 255};

#endif
