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
    _data.lastKeyPressTime = 0; // Initialize for debouncing

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
    if (_data.current_state == state_init)
    {
        // Initial setup
        _data.current_state = state_running;
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

    // Redraw UI in every loop for blinking cursor
    drawUI();

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
    auto keyboard = _data.hal->keyboard();

    // Debounce: Only process key presses if enough time has passed
    if (millis() - _data.lastKeyPressTime < _data.keyPressDelay) {
        return;
    }

    if (!keyboard->isPressed())
    {
        return;
    }
    
    keyboard->updateKeysState();

    _data.lastKeyPressTime = millis(); // Update last key press time after processing

    if (keyboard->keysState().enter)
    {
        if (!_data.calculated && _data.inputBuffer.length() > 0)
        {
            _data.cabbageWeight = std::stoi(_data.inputBuffer);
            calculate();
            _data.hal->playNextSound();
        }
        return;
    }

    if (keyboard->keysState().del)
    {
        if (!_data.calculated && _data.inputBuffer.length() > 0)
        {
            _data.inputBuffer.pop_back();
        }
        return;
    }

    for (auto &key : keyboard->keysState().values)
    {
        if (key >= '0' && key <= '9')
        {
            if (!_data.calculated && _data.inputBuffer.length() < 4)
            {
                _data.inputBuffer += key;
            }
        }
        else if (key == 'r' || key == 'R')
        {
            reset();
            _data.hal->playNextSound();
        }
        else if (key == 'c' || key == 'C')
        {
            if (_data.calculated)
            {
                _data.calculated = false;
                _data.scrollOffset = 0;
            }
            else
            {
                _data.inputBuffer.clear();
            }
        }
        else if (key == ';' || key == 0x1B)
        { // Up
            if (_data.calculated)
            {
                if (_data.scrollOffset > 0)
                {
                    _data.scrollOffset--;
                }
            }
            else
            {
                int weight = _data.inputBuffer.empty() ? 0 : std::stoi(_data.inputBuffer);
                weight = std::min(9999, weight + 100);
                _data.inputBuffer = std::to_string(weight);
            }
        }
        else if (key == '.' || key == 0x1F)
        { // Down
            if (_data.calculated)
            {
                if (_data.scrollOffset < 5)
                {
                    _data.scrollOffset++;
                }
            }
            else
            {
                int weight = _data.inputBuffer.empty() ? 0 : std::stoi(_data.inputBuffer);
                weight = std::max(0, weight - 100);
                _data.inputBuffer = std::to_string(weight);
            }
        }
    }
}

void AppKimchi::drawUI()
{
    _data.hal->canvas()->fillScreen(THEME_COLOR_BG);
    _data.hal->canvas()->setCursor(4, 2);
    _data.hal->canvas()->setTextSize(1);

    _data.hal->canvas()->printf("KIMCHI CALCULATOR\n");

    // Cabbage weight input
    _data.hal->canvas()->printf("Cabbage (g): %s", _data.inputBuffer.c_str());

    // Add a "cursor"
    if (!_data.calculated)
    {
        int16_t x = _data.hal->canvas()->getCursorX();
        int16_t y = _data.hal->canvas()->getCursorY();
        if ((millis() % 1000) < 500)
        {
            _data.hal->canvas()->fillRect(x + 2, y, 6, 8, THEME_COLOR_REPL_TEXT);
        }
    }
    _data.hal->canvas()->printf("\n");

    if (_data.calculated)
    {
        int y = 62;
        for (int i = _data.scrollOffset; i < _data.scrollOffset + 6 && i < 11; i++)
        {

            // Format ingredient display
            char buffer[64];
            if (_data.ingredients[i].isRange)
            {
                snprintf(buffer,
                         sizeof(buffer),
                         "%s: %.0f-%.0f %s",
                         _data.ingredients[i].name,
                         _data.ingredients[i].amount,
                         _data.ingredients[i].maxAmount,
                         _data.ingredients[i].unit);
            }
            else
            {
                // Show 1 decimal for pieces/heads, 0 for grams/ml
                if (strcmp(_data.ingredients[i].unit, "pcs") == 0 || strcmp(_data.ingredients[i].unit, "heads") == 0)
                {
                    snprintf(
                        buffer, sizeof(buffer), "%s: %.1f %s", _data.ingredients[i].name, _data.ingredients[i].amount, _data.ingredients[i].unit);
                }
                else
                {
                    snprintf(
                        buffer, sizeof(buffer), "%s: %.0f %s", _data.ingredients[i].name, _data.ingredients[i].amount, _data.ingredients[i].unit);
                }
            }
            _data.hal->canvas()->print(buffer);
            _data.hal->canvas()->printf("\n");
        }

        // Add a hint to go back
        _data.hal->canvas()->setCursor(4, 135 - 8 - 2); // Bottom
        _data.hal->canvas()->print("Press 'C' to change weight");
    }
    else
    {
        // Help text for input mode
        _data.hal->canvas()->setCursor(4, 62);
        _data.hal->canvas()->printf("Up/Down: Adjust weight (+/-100g)\n");
        _data.hal->canvas()->printf("Numbers: Set exact weight\n");
        _data.hal->canvas()->printf("Enter: Calculate\n");
        _data.hal->canvas()->printf("R: Reset to 2000g\n");
        _data.hal->canvas()->printf("C: Clear input\n");
    }

    _data.hal->canvas_update();
}

void AppKimchi::reset()
{
    _data.cabbageWeight = 2000;
    _data.inputBuffer = "2000";
    _data.scrollOffset = 0;
    _data.calculated = false;
}
