#include "app_kimchi.h"
#include "mooncake.h"
#include "lgfx/v1/misc/enum.hpp"

using namespace MOONCAKE::APPS;

void AppKimchi::onCreate()
{
    // Get hal
    _data.hal = (HAL::Hal*)mcAppGetDatabase()->Get("HAL")->value<HAL::Hal*>();

    // Initialize state
    _data.cabbageWeight = 2000;
    _data.inputBuffer = "2000";
    _data.scrollOffset = 0;
    _data.calculated = false;

    // Initialize ingredients array
    for (int i = 0; i < 11; i++) {
        _data.ingredients[i] = {"", 0, "", false, 0};
    }

    // Calculate default values
    calculate();
}

void AppKimchi::onResume()
{
    ANIM_APP_OPEN();

    auto canvas = _data.hal->canvas();
    canvas->fillScreen(THEME_COLOR_BG);
    canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
    canvas->setFont(FONT_REPL);
    canvas->setTextSize(FONT_SIZE_REPL);
    canvas->setCursor(0, 0);

    _data.current_state = state_init;
}

void AppKimchi::onRunning()
{
    if (_data.current_state == state_init) {
        // Initial draw and setup
        drawUI();
        _data.current_state = state_running;
        return;
    }

    // Handle home button to exit
    if (_data.hal->homeButton()->pressed())
    {
        _data.hal->playNextSound();
        destroyApp();
        return;
    }

    // Handle keyboard input
    handleInput();

    // Small delay to prevent excessive updates
    delay(50);
}

void AppKimchi::onDestroy()
{
    // Cleanup if needed
}

void AppKimchi::calculate()
{
    if (_data.cabbageWeight < 500) _data.cabbageWeight = 500;
    if (_data.cabbageWeight > 9999) _data.cabbageWeight = 9999;

    float ratio = _data.cabbageWeight / 2000.0;

    // Calculate each ingredient
    _data.ingredients[0] = {"Onion", ratio, "pcs", false, 0};
    _data.ingredients[1] = {"Carrot", (float)(_data.cabbageWeight * 0.1), "g", false, 0};
    _data.ingredients[2] = {"Green onion", (float)(_data.cabbageWeight * 0.04), "g", false, 0};
    _data.ingredients[3] = {"Daikon", (float)(_data.cabbageWeight * 0.1), "g", false, 0};
    _data.ingredients[4] = {"Garlic", ratio * 2, "heads", false, 0};
    _data.ingredients[5] = {"Ginger", (float)(_data.cabbageWeight * 0.01), "g", true, (float)(_data.cabbageWeight * 0.015)};
    _data.ingredients[6] = {"Pepper", (float)(_data.cabbageWeight * 0.045), "g", true, (float)(_data.cabbageWeight * 0.09)};
    _data.ingredients[7] = {"Sugar", (float)(_data.cabbageWeight * 0.018), "g", false, 0};
    _data.ingredients[8] = {"Rice flour", (float)(_data.cabbageWeight * 0.01), "g", false, 0};
    _data.ingredients[9] = {"Fish sauce", (float)(_data.cabbageWeight * 0.05), "ml", false, 0};
    _data.ingredients[10] = {"Salt", (float)(_data.cabbageWeight * 0.02), "g", true, (float)(_data.cabbageWeight * 0.025)};

    _data.calculated = true;
}

void AppKimchi::handleInput()
{
    // Get keyboard object
    auto keyboard = _data.hal->keyboard();

    // Check if any key is pressed
    if (keyboard->isPressed())
    {
        keyboard->updateKeysState();

        if (keyboard->keysState().enter)
        {
            if (_data.inputBuffer.length() > 0)
            {
                _data.cabbageWeight = std::stoi(_data.inputBuffer);
                calculate();
                _data.hal->playNextSound();
                drawUI(); // Redraw after calculation
            }
        }
        else if (keyboard->keysState().del)
        {
            if (_data.inputBuffer.length() > 0)
            {
                _data.inputBuffer.pop_back();
                drawUI(); // Redraw after deletion
            }
        }
        else
        {
            for (auto& key : keyboard->keysState().values)
            {
                if (key >= '0' && key <= '9')
                {
                    if (_data.inputBuffer.length() < 4)
                    { // Max 4 digits (9999g)
                        _data.inputBuffer += key;
                        drawUI(); // Redraw after adding digit
                    }
                }
                else if (key == 'r' || key == 'R')
                {
                    reset();
                    _data.hal->playNextSound();
                    drawUI(); // Redraw after reset
                }
                else if (key == 'c' || key == 'C')
                {
                    _data.inputBuffer.clear();
                    drawUI(); // Redraw after clearing
                }
                else if (key == ';' || key == 0x1B)
                { // Up arrow
                    if (_data.scrollOffset > 0)
                    {
                        _data.scrollOffset--;
                        drawUI(); // Redraw after scrolling
                    }
                }
                else if (key == '.' || key == 0x1F)
                { // Down arrow
                    if (_data.scrollOffset < 5)
                    { // Max scroll (11 items - 6 visible)
                        _data.scrollOffset++;
                        drawUI(); // Redraw after scrolling
                    }
                }
            }
        }
    }
}

void AppKimchi::drawUI()
{
    _data.hal->canvas()->fillScreen(THEME_COLOR_BG);

    _data.hal->canvas()->setCursor(4, 2);
    // _data.hal->canvas()->setCursor(10, 5);
    _data.hal->canvas()->setTextSize(1);

    _data.hal->canvas()->printf("KIMCHI CALCULATOR\n");

    // Ingredients list (scrollable, show 6 at a time)
    if (_data.calculated) {
        int y = 62;
        for (int i = _data.scrollOffset; i < _data.scrollOffset + 6 && i < 11; i++) {

            // Format ingredient display
            char buffer[64];
            if (_data.ingredients[i].isRange) {
                snprintf(buffer, sizeof(buffer), "%s: %.0f-%.0f %s",
                    _data.ingredients[i].name,
                    _data.ingredients[i].amount,
                    _data.ingredients[i].maxAmount,
                    _data.ingredients[i].unit);
            } else {
                // Show 1 decimal for pieces/heads, 0 for grams/ml
                if (strcmp(_data.ingredients[i].unit, "pcs") == 0 ||
                    strcmp(_data.ingredients[i].unit, "heads") == 0) {
                    snprintf(buffer, sizeof(buffer), "%s: %.1f %s",
                        _data.ingredients[i].name,
                        _data.ingredients[i].amount,
                        _data.ingredients[i].unit);
                } else {
                    snprintf(buffer, sizeof(buffer), "%s: %.0f %s",
                        _data.ingredients[i].name,
                        _data.ingredients[i].amount,
                        _data.ingredients[i].unit);
                }
            }
            _data.hal->canvas()->print(buffer);
            _data.hal->canvas()->printf("\n");
            //y += 13;
        }

        // Scroll indicator with more conservative right margin
        //if (_data.scrollOffset > 0) {
        //    canvas->drawString("^", canvas->width() - 20, 42);
        //}
        //if (_data.scrollOffset < 5) {
        //    canvas->drawString("v", canvas->width() - 20, y - 13); // Position based on last drawn line
        //}
    } else {
        _data.hal->canvas()->printf("Enter cabbage weight & press ENTER\n");
    }
    _data.hal->canvas_update(); 
}

void AppKimchi::reset()
{
    _data.cabbageWeight = 2000;
    _data.inputBuffer = "2000";
    _data.scrollOffset = 0;
    calculate();
}
