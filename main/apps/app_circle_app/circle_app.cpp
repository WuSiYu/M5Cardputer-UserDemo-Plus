#include "circle_app.h"
#include "../../hal/hal.h"

using namespace MOONCAKE::APPS;

void CircleApp::onCreate()
{
    HAL::GetDisplay()->fillRect(0, 0, HAL::GetDisplay()->width(), HAL::GetDisplay()->height(), TFT_BLACK); // Clear screen
    int centerX = HAL::GetDisplay()->width() / 2;
    int centerY = HAL::GetDisplay()->height() / 2;
    int radius = 30;
    HAL::GetDisplay()->fillCircle(centerX, centerY, radius, TFT_WHITE);
    HAL::GetDisplay()->pushSprite(0, 0);
}

void CircleApp::onResume() {}
void CircleApp::onRunning() {}
void CircleApp::onRunningBG() {}
void CircleApp::onPause() {}
void CircleApp::onDestroy() {}