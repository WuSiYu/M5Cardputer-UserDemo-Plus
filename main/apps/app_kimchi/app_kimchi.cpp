#include "app_kimchi.h"
#include "mooncake.h"

using namespace MOONCAKE::APPS;

void AppKimchi::onCreate()
{
    _hal = (HAL::Hal*)mcAppGetDatabase()->Get("HAL")->value<HAL::Hal*>();
    
    // Initialize state
    _cabbageWeight = 2000;
    _inputBuffer = "2000";
    _scrollOffset = 0;
    _calculated = false;
    
    // Initialize ingredients array
    for (int i = 0; i < 11; i++) {
        _ingredients[i] = {"", 0, "", false, 0};
    }
    
    // Calculate default values
    calculate();
    
    // Initial draw
    drawUI();
}

void AppKimchi::onResume()
{
    drawUI();
}

void AppKimchi::onRunning()
{
    // Handle home button to exit
    if (_hal->homeButton()->pressed())
    {
        _hal->playNextSound();
        destroyApp();
        return;
    }
    
    // Handle keyboard input
    handleInput();
    
    // Redraw UI
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
    if (_cabbageWeight < 500) _cabbageWeight = 500;
    if (_cabbageWeight > 9999) _cabbageWeight = 9999;
    
    float ratio = _cabbageWeight / 2000.0;
    
    // Calculate each ingredient
    _ingredients[0] = {"Onion", ratio, "pcs", false, 0};
    _ingredients[1] = {"Carrot", (float)(_cabbageWeight * 0.1), "g", false, 0};
    _ingredients[2] = {"Green onion", (float)(_cabbageWeight * 0.04), "g", false, 0};
    _ingredients[3] = {"Daikon", (float)(_cabbageWeight * 0.1), "g", false, 0};
    _ingredients[4] = {"Garlic", ratio * 2, "heads", false, 0};
    _ingredients[5] = {"Ginger", (float)(_cabbageWeight * 0.01), "g", true, (float)(_cabbageWeight * 0.015)};
    _ingredients[6] = {"Pepper", (float)(_cabbageWeight * 0.045), "g", true, (float)(_cabbageWeight * 0.09)};
    _ingredients[7] = {"Sugar", (float)(_cabbageWeight * 0.018), "g", false, 0};
    _ingredients[8] = {"Rice flour", (float)(_cabbageWeight * 0.01), "g", false, 0};
    _ingredients[9] = {"Fish sauce", (float)(_cabbageWeight * 0.05), "ml", false, 0};
    _ingredients[10] = {"Salt", (float)(_cabbageWeight * 0.02), "g", true, (float)(_cabbageWeight * 0.025)};
    
    _calculated = true;
}

void AppKimchi::handleInput()
{
    // Get keyboard object
    auto keyboard = _hal->keyboard();

    // Check if any key is pressed
    if (keyboard->isPressed())
    {
        keyboard->updateKeysState();

        if (keyboard->keysState().enter)
        {
            if (_inputBuffer.length() > 0)
            {
                _cabbageWeight = std::stoi(_inputBuffer);
                calculate();
                _hal->playNextSound();
            }
        }
        else if (keyboard->keysState().del)
        {
            if (_inputBuffer.length() > 0)
            {
                _inputBuffer.pop_back();
            }
        }
        else
        {
            for (auto& key : keyboard->keysState().values)
            {
                if (key >= '0' && key <= '9')
                {
                    if (_inputBuffer.length() < 4)
                    { // Max 4 digits (9999g)
                        _inputBuffer += key;
                    }
                }
                else if (key == 'r' || key == 'R')
                {
                    reset();
                    _hal->playNextSound();
                }
                else if (key == 'c' || key == 'C')
                {
                    _inputBuffer.clear();
                }
                else if (key == ';' || key == 0x1B)
                { // Up arrow
                    if (_scrollOffset > 0)
                    {
                        _scrollOffset--;
                    }
                }
                else if (key == '.' || key == 0x1F)
                { // Down arrow
                    if (_scrollOffset < 5)
                    { // Max scroll (11 items - 6 visible)
                        _scrollOffset++;
                    }
                }
            }
        }
    }
}

void AppKimchi::drawUI()
{
    auto canvas = _hal->canvas();
    
    // Clear screen
    canvas->fillScreen(TFT_BLACK);
    canvas->setTextColor(TFT_WHITE, TFT_BLACK);
    
    // Title
    canvas->setTextSize(1);
    canvas->drawString("KIMCHI CALCULATOR", 10, 5);
    
    // Input field with cursor
    canvas->drawString("Cabbage:", 10, 22);
    std::string inputDisplay = _inputBuffer;
    if (millis() % 1000 < 500) {  // Blinking cursor
        inputDisplay += "_";
    }
    inputDisplay += " g";
    canvas->drawString(inputDisplay.c_str(), 75, 22);
    
    // Separator line
    canvas->drawLine(5, 35, 235, 35, TFT_WHITE);
    
    // Ingredients list (scrollable, show 6 at a time)
    if (_calculated) {
        int y = 42;
        for (int i = _scrollOffset; i < _scrollOffset + 6 && i < 11; i++) {
            canvas->setCursor(10, y);
            
            // Format ingredient display
            char buffer[64];
            if (_ingredients[i].isRange) {
                snprintf(buffer, sizeof(buffer), "%s: %.0f-%.0f %s", 
                    _ingredients[i].name,
                    _ingredients[i].amount,
                    _ingredients[i].maxAmount,
                    _ingredients[i].unit);
            } else {
                // Show 1 decimal for pieces/heads, 0 for grams/ml
                if (strcmp(_ingredients[i].unit, "pcs") == 0 || 
                    strcmp(_ingredients[i].unit, "heads") == 0) {
                    snprintf(buffer, sizeof(buffer), "%s: %.1f %s", 
                        _ingredients[i].name,
                        _ingredients[i].amount,
                        _ingredients[i].unit);
                } else {
                    snprintf(buffer, sizeof(buffer), "%s: %.0f %s", 
                        _ingredients[i].name,
                        _ingredients[i].amount,
                        _ingredients[i].unit);
                }
            }
            canvas->print(buffer);
            y += 13;
        }
        
        // Scroll indicator
        if (_scrollOffset > 0) {
            canvas->drawString("^", 230, 42);
        }
        if (_scrollOffset < 5) {
            canvas->drawString("v", 230, 110);
        }
    } else {
        canvas->drawString("Enter weight & press ENTER", 10, 60);
    }
    
    // Footer with instructions
    canvas->drawString("HOME:Exit R:Reset", 10, 122);
    
    // Push to display
    canvas->pushSprite(0, 0);
}

void AppKimchi::reset()
{
    _cabbageWeight = 2000;
    _inputBuffer = "2000";
    _scrollOffset = 0;
    calculate();
}