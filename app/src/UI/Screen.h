// Screen.h - one on-screen thing the app can be showing: the WORLDS tab, the
// DEFAULTS tab, or the world editor reached from the first of them. A Screen
// only ever sees Platform's Renderer and ButtonEdges - never SDL2, never
// OpenOrbis - and never owns app-wide state directly.
#pragma once

#include "../Platform/Input.h"
#include "../Platform/Renderer.h"

#include <string>

namespace bbr {

// Every screen the app can show. Deliberately a flat enum, not a generic
// "next screen" pointer/factory - not enough screens exist yet to justify a
// back-stack or transition history. This is the smallest thing that lets a
// screen ask for a different one; grow it if a real need shows up.
enum class ScreenId {
    None,   // no transition requested

    // The main screen's two tabs, and the editor reached from the first of
    // them (worlds B1, B6, B7). These three are the whole app.
    //
    // Four ids the worlds model replaced were removed in milestone 6. The
    // main menu became the WORLDS tab, the Enable wizard became the world
    // editor (worlds plan P8), Disable became activating Vanilla (B8, B27),
    // and the save-data probe was the harness that hardware-tested
    // milestones 1-3 and went with them.
    Worlds,
    Defaults,
    WorldEditor,
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

    // Which world ScreenId::WorldEditor is being asked for. Empty means a NEW
    // world, pre-filled from Defaults (worlds B6); a "w-NNNN" means that
    // world's current settings (B7). Read by Application in the same pass as
    // RequestedScreen(), because the screen that answered is destroyed
    // immediately afterwards.
    virtual std::string RequestedWorldId() const { return std::string(); }
};

} // namespace bbr
