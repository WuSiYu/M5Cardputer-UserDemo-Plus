#pragma once
#include "mooncake.h"
#include "../../hal/hal.h"
#include "../utils/theme/theme_define.h"

namespace MOONCAKE
{
    namespace APPS
    {
        class CircleApp : public APP_BASE
        {
            public:
                // App name and icon
                std::string getAppName() override { return "Circle App"; }
                void* getAppIcon() override { return (void*)THEME_APP_ICON_DEFAULT; }

                // Lifecycle methods
                void onCreate() override;
                void onResume() override;
                void onRunning() override;
                void onRunningBG() override;
                void onPause() override;
                void onDestroy() override;
        };

        class CircleApp_Packer : public APP_PACKER_BASE
        {
            public:
                std::string getAppName() override { return "Circle App"; }
                void* getAppIcon() override { return (void*)THEME_APP_ICON_DEFAULT; }
                void* newApp() override { return new CircleApp; }
                void deleteApp(void *app) override { delete (CircleApp*)app; }
        };
    }
}