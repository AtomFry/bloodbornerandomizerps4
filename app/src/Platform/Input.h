// Input.h - abstracts the PS4 pad behind plain button-edge flags, so Screens
// never see SDL_Event or SDL_Joystick.
#pragma once

typedef union SDL_Event SDL_Event;
typedef struct _SDL_Joystick SDL_Joystick;

namespace bbr {

// Newly-pressed-this-frame flags.
struct ButtonEdges {
    bool cross    = false;
    bool circle   = false;
    bool square   = false;
    bool triangle = false;
    bool up       = false;
    bool down     = false;
    bool left     = false;
    bool right    = false;

    // L1/R1 page a long list a screenful at a time - see UI/EnemyPicker.h.
    // Nothing else uses them.
    bool l1       = false;
    bool r1       = false;

    bool options  = false;
};

class Input {
public:
    // D-pad: wired via SDL_JOYHATMOTION and the standard SDL_HAT_* bitmask.
    // Confirmed working on hardware in M3 testing (live.log showed
    // hat=0 value=0x01/0x04 for up/down) - this is now the only menu
    // navigation; Square/Triangle were dropped in M4 once d-pad was proven
    // out, since having two ways to do the same thing wasn't intuitive.
    // Every hat/button event is still logged raw for future debugging.

    // Opens the first joystick, if any. Not finding one isn't fatal - the
    // app still runs, just without input, same as M1.
    void Open();
    void Close();

    // Call once per frame before feeding it events.
    void BeginFrame();

    // Call once per SDL_PollEvent result. Updates the current frame's edges
    // on SDL_JOYBUTTONDOWN; ignores everything else.
    void HandleEvent(const SDL_Event& ev);

    const ButtonEdges& Current() const { return edges_; }

private:
    SDL_Joystick* joystick_ = nullptr;
    ButtonEdges   edges_;
};

} // namespace bbr
