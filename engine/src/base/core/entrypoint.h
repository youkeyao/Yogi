#pragma once

int main(int argc, char** argv)
{
    Yogi::Log::init();

    YG_PROFILE_BEGIN_SESSION("startup", "Yogi_profile_startup.json");
    auto app = Yogi::create_application();
    YG_PROFILE_END_SESSION();

    YG_PROFILE_BEGIN_SESSION("runtime", "Yogi_profile_runtime.json");
    app->run();
    YG_PROFILE_END_SESSION();
    
    YG_PROFILE_BEGIN_SESSION("shutdown", "Yogi_profile_shutdown.json");
    delete app;
    YG_PROFILE_END_SESSION();

    return 0;
}