/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <hal/hal.h>
#include "raylib.h"
#include <apps/utils/theme.h>
#include <assets/assets.h>
#include <cstdint>

void InitWindow(int w, int h, const char* title)
{
    GetHAL().canvas.setFont(FONT_REPL);
}

void SetTargetFPS(int fps)
{
}

bool WindowShouldClose()
{
    return false;
}

void CloseWindow()
{
}

int GetScreenWidth()
{
    return GetHAL().canvas.width();
}

void BeginDrawing()
{
}

void EndDrawing()
{
    GetHAL().pushCanvas();
}

void ClearBackground(uint32_t color)
{
    GetHAL().canvas.fillScreen(color);
}

void DrawRectangle(int x, int y, int w, int h, uint32_t color)
{
    GetHAL().canvas.fillRect(x, y, w, h, color);
}

void DrawCircleV(Vector2 center, float r, uint32_t color)
{
    GetHAL().canvas.fillSmoothCircle(center.x, center.y, r, color);
}

// Draw a color-filled circle
void DrawCircle(int centerX, int centerY, float radius, uint32_t color)
{
    DrawCircleV((Vector2){(float)centerX, (float)centerY}, radius, color);
}

void DrawLineV(Vector2 startPos, Vector2 endPos, uint32_t color)
{
    GetHAL().canvas.drawLine(startPos.x, startPos.y, endPos.x, endPos.y, color);
}

void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, uint32_t color)
{
    GetHAL().canvas.drawLine(startPosX, startPosY, endPosX, endPosY, color);
}

void DrawRectangleV(Vector2 position, Vector2 size, uint32_t color)
{
    GetHAL().canvas.fillRect(position.x, position.y, size.x, size.y, color);
}

void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, uint32_t color)
{
    GetHAL().canvas.fillTriangle(v1.x, v1.y, v2.x, v2.y, v3.x, v3.y, color);
}

// #include <random>
// std::random_device rd;
// std::mt19937 gen(rd());
// int GetRandomValue(int min, int max)
// {
//     std::uniform_int_distribution<> dist(min, max);
//     return dist(gen);
// }

int GetRandomValue(int min, int max)
{
    // Simple random implementation to save DIRAM
    int rnd = static_cast<int>(GetHAL().millis() % 0xFFFFFFFF);
    rnd = (rnd * 1103515245 + 12345) & 0x7FFFFFFF;
    return min + (rnd % (max - min + 1));
}

// Check collision between two circles
bool CheckCollisionCircles(Vector2 center1, float radius1, Vector2 center2, float radius2)
{
    bool collision = false;

    float dx = center2.x - center1.x;  // X distance between centers
    float dy = center2.y - center1.y;  // Y distance between centers

    float distance = sqrtf(dx * dx + dy * dy);  // Distance between centers

    if (distance <= (radius1 + radius2)) collision = true;

    return collision;
}

// Check collision between circle and rectangle
// NOTE: Reviewed version to take into account corner limit case
bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec)
{
    bool collision = false;

    int recCenterX = (int)(rec.x + rec.width / 2.0f);
    int recCenterY = (int)(rec.y + rec.height / 2.0f);

    float dx = fabsf(center.x - (float)recCenterX);
    float dy = fabsf(center.y - (float)recCenterY);

    if (dx > (rec.width / 2.0f + radius)) {
        return false;
    }
    if (dy > (rec.height / 2.0f + radius)) {
        return false;
    }

    if (dx <= (rec.width / 2.0f)) {
        return true;
    }
    if (dy <= (rec.height / 2.0f)) {
        return true;
    }

    float cornerDistanceSq =
        (dx - rec.width / 2.0f) * (dx - rec.width / 2.0f) + (dy - rec.height / 2.0f) * (dy - rec.height / 2.0f);

    collision = (cornerDistanceSq <= (radius * radius));

    return collision;
}
