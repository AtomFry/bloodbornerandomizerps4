// Screen.h - one on-screen thing the app can be showing (main menu, the
// Enable/Disable wizards, Setup Defaults, ...). A Screen only ever sees
// Platform's Renderer and ButtonEdges - never SDL2, never OpenOrbis - and
// never owns app-wide state directly. See UI_BLUEPRINT.md for the full
// screen inventory this is being built toward.
#pragma once

#include "../Platform/Input.h"
#include "../Platform/Renderer.h"

namespace bbr {

// Every screen the app can show. Deliberately a flat enum, not a generic
// "next screen" pointer/factory - not enough screens exist yet to justify a
// back-stack or transition history. This is the smallest thing that lets a
// screen ask for a different one; grow it if a real need shows up.
enum class ScreenId {
    None,   // no transition requested
    Menu,
    EnableWizard,
    DisableWizard,
    SetupDefaults,
};

class Screen {
public:
    virtual ~Screen() = default;

    virtual void Update(const ButtonEdges& input) = 0;
    virtual void Draw(Renderer& renderer) = 0;

    // True once this screen wants the whole app to end (e.g. O pressed at
    // the top-level menu).
    virtual bool WantsExit() const { return false; }

    // Non-None once this screen wants control handed to a different one
    // (e.g. O pressed inside a submenu, to go back). Checked once per frame
    // by Application, after Update() - see Application.cpp.
    virtual ScreenId RequestedScreen() const { return ScreenId::None; }
};

} // namespace bbr
