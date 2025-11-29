#pragma once
#include <mooncake.h>
#include "hal/hal.h"
#include <string>
#include <vector>

namespace MOONCAKE
{
    namespace APPS
    {
        class AppKimchi : public APP_BASE
        {
        public:
            void onCreate() override;
            void onResume() override;
            void onRunning() override;
            void onDestroy() override;

        private:
            struct Ingredient
            {
                std::string name;
                std::string amount;
                std::string notes;
            };

            static void _update_input_label(lv_event_t* e);
            static void _lvgl_event_cb(lv_event_t* e);
            void _update_ingredients();

            float _cabbage_weight = 2000.0f;
            std::vector<Ingredient> _ingredients;
            HAL::HalCardputer* _hal;
            
            struct
            {
                lv_obj_t* screen = nullptr;
                lv_obj_t* cabbage_input = nullptr;
                lv_obj_t* ingredients_table = nullptr;
                lv_obj_t* keyboard = nullptr;
            } _ui;
        };

        class AppKimchi_packer : public APP_PACKER_BASE
        {
            std::string getAppName() override;
            void* getAppIcon() override;
            void* newApp() override { return new AppKimchi; }
            void deleteApp(void* app) override { delete (AppKimchi*)app; }
        };
    }
}
