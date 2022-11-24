#pragma once

int main(int argc, char** argv)
{
    auto app = hazel::create_application();
    app->run();
    delete app;
}