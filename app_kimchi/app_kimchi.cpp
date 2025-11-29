#include "app_kimchi.h"

using namespace MOONCAKE::APPS;

std::string AppKimchi_packer::getAppName()
{
    return "Kimchi Calc";
}

void* AppKimchi_packer::getAppIcon()
{
    return nullptr;
}

void AppKimchi::onCreate()
{
    // Get hal
    _hal = (HAL::HalCardputer*)mcAppGetDatabase()->Get("HAL")->value<HAL::Hal*>();
    
    _ui.screen = lv_obj_create(NULL);
    lv_scr_load_anim(_ui.screen, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);

    lv_obj_t* title = lv_label_create(_ui.screen);
    lv_label_set_text(title, "Kimchi Calculator");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Cabbage weight input
    lv_obj_t* cabbage_label = lv_label_create(_ui.screen);
    lv_label_set_text(cabbage_label, "Cabbage (g):");
    lv_obj_align_to(cabbage_label, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

    _ui.cabbage_input = lv_textarea_create(_ui.screen);
    lv_textarea_set_one_line(_ui.cabbage_input, true);
    lv_textarea_set_text(_ui.cabbage_input, "2000");
    lv_obj_set_width(_ui.cabbage_input, 100);
    lv_obj_align_to(_ui.cabbage_input, cabbage_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(_ui.cabbage_input, _update_input_label, LV_EVENT_ALL, this);

    // Ingredients table
    _ui.ingredients_table = lv_table_create(_ui.screen);
    lv_obj_align_to(_ui.ingredients_table, cabbage_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    lv_table_set_col_width(_ui.ingredients_table, 0, 80);
    lv_table_set_col_width(_ui.ingredients_table, 1, 80);
    lv_table_set_col_width(_ui.ingredients_table, 2, 80);
    
    // Keyboard
    _ui.keyboard = lv_keyboard_create(_ui.screen);
    lv_keyboard_set_textarea(_ui.keyboard, _ui.cabbage_input);
    lv_obj_add_flag(_ui.keyboard, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_add_event_cb(_ui.cabbage_input, _lvgl_event_cb, LV_EVENT_ALL, this);
    
    _update_ingredients();
}

void AppKimchi::onResume()
{
}

void AppKimchi::onRunning()
{
    _hal->keyboard.update();
}

void AppKimchi::onDestroy()
{
    lv_obj_del(_ui.screen);
}

void AppKimchi::_update_input_label(lv_event_t* e)
{
    AppKimchi* app = (AppKimchi*)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* ta = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        app->_cabbage_weight = atof(lv_textarea_get_text(ta));
        app->_update_ingredients();
    }
}

void AppKimchi::_lvgl_event_cb(lv_event_t* e)
{
    AppKimchi* app = (AppKimchi*)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* ta = lv_event_get_target(e);

    if (code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(app->_ui.keyboard, ta);
        lv_obj_clear_flag(app->_ui.keyboard, LV_OBJ_FLAG_HIDDEN);
    } else if (code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(app->_ui.keyboard, NULL);
        lv_obj_add_flag(app->_ui.keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

// Based on the JS from the HTML file
void AppKimchi::_update_ingredients()
{
    _ingredients.clear();
    
    float ratio = _cabbage_weight / 2000.0f;

    _ingredients.push_back({"Cabbage", std::to_string((int)_cabbage_weight) + " g", "1 head (approx)"});

    // Onion
    float onion_amount = 1.0f * ratio;
    std::string onion_text;
    if (onion_amount < 1.2)
        onion_text = "1 piece";
    else if (onion_amount < 2)
        onion_text = "1.5-2 pieces";
    else
        onion_text = std::to_string(onion_amount) + " pieces";
    _ingredients.push_back({"Onion", onion_text, ""});
    
    _ingredients.push_back({"Carrot", std::to_string((int)(200 * ratio)) + " g", ""});
    _ingredients.push_back({"Green onion", std::to_string((int)(80 * ratio)) + " g", ""});
    _ingredients.push_back({"Daikon radish", std::to_string((int)(200 * ratio)) + " g", ""});

    // Garlic
    float garlic_amount = 2.0f * ratio;
    std::string garlic_text;
    if (garlic_amount < 1)
        garlic_text = std::to_string(garlic_amount) + " head";
    else
        garlic_text = std::to_string(garlic_amount) + " heads";
    _ingredients.push_back({"Garlic", garlic_text, "Approx " + std::to_string((int)(garlic_amount * 12)) + " cloves"});

    _ingredients.push_back({"Ginger", std::to_string((int)(20 * ratio)) + "-" + std::to_string((int)(30 * ratio)) + " g", ""});
    _ingredients.push_back({"Gochugaru", std::to_string((int)(90 * ratio)) + "-" + std::to_string((int)(180 * ratio)) + " g", "Adjust to taste"});
    
    // Sugar
    int sugar_grams = (int)(36 * ratio);
    std::string sugar_text;
    if (sugar_grams <= 50)
        sugar_text = std::to_string((int)(sugar_grams / 12)) + " tbsp (~" + std::to_string(sugar_grams) + " g)";
    else
        sugar_text = std::to_string(sugar_grams) + " g";
    _ingredients.push_back({"Sugar", sugar_text, ""});

    // Rice flour
    int rice_flour_grams = (int)(20 * ratio);
    std::string rice_flour_text;
    if (rice_flour_grams <= 40)
        rice_flour_text = std::to_string((int)(rice_flour_grams / 10)) + " tbsp (~" + std::to_string(rice_flour_grams) + " g)";
    else
        rice_flour_text = std::to_string(rice_flour_grams) + " g";
    _ingredients.push_back({"Rice flour", rice_flour_text, ""});

    _ingredients.push_back({"Fish sauce", std::to_string((int)(100 * ratio)) + " ml", ""});

    // Salt
    int salt_min = (int)(_cabbage_weight * 2.0 / 100.0);
    int salt_max = (int)(_cabbage_weight * 2.5 / 100.0);
    _ingredients.push_back({"Salt", std::to_string(salt_min) + "-" + std::to_string(salt_max) + " g", "For brining"});

    // Update table
    lv_table_set_row_cnt(_ui.ingredients_table, _ingredients.size() + 1);
    lv_table_set_cell_value(_ui.ingredients_table, 0, 0, "Ingredient");
    lv_table_set_cell_value(_ui.ingredients_table, 0, 1, "Amount");
    lv_table_set_cell_value(_ui.ingredients_table, 0, 2, "Notes");

    for (int i = 0; i < _ingredients.size(); i++)
    {
        lv_table_set_cell_value(_ui.ingredients_table, i + 1, 0, _ingredients[i].name.c_str());
        lv_table_set_cell_value(_ui.ingredients_table, i + 1, 1, _ingredients[i].amount.c_str());
        lv_table_set_cell_value(_ui.ingredients_table, i + 1, 2, _ingredients[i].notes.c_str());
    }
}
