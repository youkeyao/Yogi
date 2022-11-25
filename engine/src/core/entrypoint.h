#pragma once

int main(int argc, char** argv)
{
    hazel::Log::init();

    auto app = hazel::create_application();
    app->run();
    delete app;

    return 0;
}