// Bloodborne Randomizer - entry point.
//
// M2: main() now only hands off to Application. Everything that used to be
// here directly (SDL2 init, the render loop, exit handling) moved into
// Platform/UI/Application - see those for the M1 behavior and the notes on
// why exit works the way it does.
#include "Application.h"

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    bbr::Application app;
    app.Run(); // never returns
}
