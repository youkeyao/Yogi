#pragma once

int main(int argc, char** argv)
{
    hazel::Log::init();

    HZ_PROFILE_BEGIN_SESSION("startup", "hazel_profile_startup.json");
    auto app = hazel::create_application();
    HZ_PROFILE_END_SESSION();

    HZ_PROFILE_BEGIN_SESSION("runtime", "hazel_profile_runtime.json");
    app->run();
    HZ_PROFILE_END_SESSION();
    
    HZ_PROFILE_BEGIN_SESSION("shutdown", "hazel_profile_shutdown.json");
    delete app;
    HZ_PROFILE_END_SESSION();

    return 0;
}