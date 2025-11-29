#include "app_kimchi.h"
#include "mooncake.h"

using namespace MOONCAKE::APPS;

void AppKimchi::onCreate()
{
    _hal = (HAL::Hal*)mcAppGetDatabase()->Get("HAL")->value<HAL::Hal*>();
    _hal->canvas()->drawString("Kimchi App", 50, 50);
    _hal->canvas()->pushSprite(0, 0);
}

void AppKimchi::onResume()
{
}

void AppKimchi::onRunning()
{
    if (_hal->homeButton()->pressed())
    {
        _hal->playNextSound();
        destroyApp();
    }
}

void AppKimchi::onDestroy()
{
}
