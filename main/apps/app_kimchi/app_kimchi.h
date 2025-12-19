#pragma once
#include <mooncake.h>
#include "hal/hal.h"
#include <string>
#include <vector>
#include "../utils/theme/theme_define.h"
#include "../utils/anim/anim_define.h"
#include "../utils/icon/icon_define.h"
#include "assets/kimchi_big.h"
#include "assets/kimchi_small.h"

namespace MOONCAKE
{
    namespace APPS
    {
        struct Ingredient {
            const char* name;
            float amount;
            const char* unit;
            bool isRange;
            float maxAmount;
        };

        class AppKimchi : public APP_BASE
        {
        private:
            enum State_t
            {
                state_init = 0,
                state_running,
            };

            struct Data_t
            {
                HAL::Hal* hal = nullptr;
                State_t current_state = state_init;
                int cabbageWeight;
                std::string inputBuffer;
                Ingredient ingredients[11];
                int scrollOffset;
                bool calculated;
            };
            Data_t _data;

        public:
            void onCreate() override;
            void onResume() override;
            void onRunning() override;
            void onDestroy() override;

        private:
            void calculate();
            void handleInput();
            void drawUI();
            void reset();
        };

        class AppKimchi_packer : public APP_PACKER_BASE
        {
            std::string getAppName() override { return "Kimchi Calc"; }
            void* getAppIcon() override { return (void*)(new AppIcon_t(image_data_kimchi_big, image_data_kimchi_small)); }
            void* newApp() override { return new AppKimchi; }
            void deleteApp(void* app) override { delete (AppKimchi*)app; }
        };
    }
}