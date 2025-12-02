#pragma once
#include <mooncake.h>
#include "hal/hal.h"
#include <string>
#include <vector>
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
        public:
            void onCreate() override;
            void onResume() override;
            void onRunning() override;
            void onDestroy() override;

        private:
            HAL::Hal* _hal;
            int _cabbageWeight;
            std::string _inputBuffer;
            Ingredient _ingredients[11];
            int _scrollOffset;
            bool _calculated;
            
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