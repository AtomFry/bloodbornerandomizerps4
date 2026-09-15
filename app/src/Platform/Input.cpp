#include "Input.h"

#include "Log.h"

#include <SDL.h>
#include <stdio.h>

namespace bbr {

namespace {
// From samples/SDL2/SDL2/Game.h - this SDL2 port's low-level joystick button
// indices, confirmed correct on hardware.
// Transcribed in full from samples/SDL2/SDL2/Game.h. The first four were
// confirmed on hardware directly; the rest are corroborated by
// this project's own logs, which recorded indices 13 and 14 for d-pad
// up/down - exactly what the SDK header says. That settles the doubt the old
// comment here recorded: the ordering is NOT the common PS4 HID one (which
// would put the d-pad on a hat only and L1/R1 at 4/5 after a different face
// order), it is this SDL port's own, and OPTIONS = 9 was right.
enum PadButton {
    PAD_BUTTON_CROSS = 0,
    PAD_BUTTON_CIRCLE,
    PAD_BUTTON_SQUARE,
    PAD_BUTTON_TRIANGLE,
    PAD_BUTTON_L1,           // 4
    PAD_BUTTON_R1,           // 5
    PAD_BUTTON_OPTIONS = 9,
    PAD_BUTTON_L3     = 11,
    PAD_BUTTON_R3,           // 12
    PAD_BUTTON_UP,           // 13 - arrives as a hat event in practice
    PAD_BUTTON_DOWN,         // 14
    PAD_BUTTON_LEFT,         // 15
    PAD_BUTTON_RIGHT,        // 16
    PAD_BUTTON_TOUCH_PAD,    // 17
    PAD_BUTTON_L2,           // 18
    PAD_BUTTON_R2,           // 19
};
} // namespace

void Input::Open() {
    if (SDL_NumJoysticks() < 1) return;
    joystick_ = SDL_JoystickOpen(0);
}

void Input::Close() {
    if (joystick_) SDL_JoystickClose(joystick_);
    joystick_ = nullptr;
}

void Input::BeginFrame() {
    edges_ = ButtonEdges();
}

void Input::HandleEvent(const SDL_Event& ev) {
    if (ev.type == SDL_JOYHATMOTION) {
        char msg[64];
        snprintf(msg, sizeof(msg), "joy hat motion: hat=%d value=0x%02x", ev.jhat.hat, ev.jhat.value);
        Log(msg);

        if (ev.jhat.value & SDL_HAT_UP)    edges_.up    = true;
        if (ev.jhat.value & SDL_HAT_DOWN)  edges_.down  = true;
        if (ev.jhat.value & SDL_HAT_LEFT)  edges_.left  = true;
        if (ev.jhat.value & SDL_HAT_RIGHT) edges_.right = true;
        return;
    }

    if (ev.type != SDL_JOYBUTTONDOWN) return;

    char msg[64];
    snprintf(msg, sizeof(msg), "joy button down: index=%d", ev.jbutton.button);
    Log(msg);

    switch (ev.jbutton.button) {
        case PAD_BUTTON_CROSS:    edges_.cross    = true; break;
        case PAD_BUTTON_CIRCLE:   edges_.circle   = true; break;
        case PAD_BUTTON_SQUARE:   edges_.square   = true; break;
        case PAD_BUTTON_TRIANGLE: edges_.triangle = true; break;
        case PAD_BUTTON_L1:       edges_.l1       = true; break;
        case PAD_BUTTON_R1:       edges_.r1       = true; break;
        case PAD_BUTTON_OPTIONS:  edges_.options  = true; break;
        default: break; // unmapped index - the log line above still records it
    }
}

} // namespace bbr
