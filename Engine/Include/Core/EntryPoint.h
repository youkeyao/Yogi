#pragma once

int main(int argc, char** argv)
{
    YG_PROFILE_BEGIN_SESSION("startup", "Yogi_profile_startup.json");
    auto app = Yogi::CreateApplication();
    YG_PROFILE_END_SESSION();

    YG_PROFILE_BEGIN_SESSION("runtime", "Yogi_profile_runtime.json");
    app->Run();
    YG_PROFILE_END_SESSION();

    YG_PROFILE_BEGIN_SESSION("shutdown", "Yogi_profile_shutdown.json");
    delete app;
    YG_PROFILE_END_SESSION();

    return 0;
}