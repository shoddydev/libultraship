#include "ship/window/gui/StatsWindow.h"
#include <imgui.h>
#include "spdlog/spdlog.h"
#include "ship/Context.h"
#include "ship/window/Window.h"

namespace Ship {

// Changed from static to a file-level global so it can be shared
bool gStatsTimerStarted = false;

StatsWindow::~StatsWindow() {
    SPDLOG_TRACE("destruct stats window");
}

void StatsWindow::InitElement() {
}

void StatsWindow::DrawElement() {
    const float framerate = ImGui::GetIO().Framerate;
    const float deltatime = ImGui::GetIO().DeltaTime;
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

#if defined(_WIN32)
    ImGui::Text("Platform: Windows");
#elif defined(__IOS__)
    ImGui::Text("Platform: iOS");
#elif defined(__APPLE__)
    ImGui::Text("Platform: macOS");
#elif defined(__linux__)
    ImGui::Text("Platform: PSC");
#else
    ImGui::Text("Platform: Unknown");
#endif
    ImGui::Text("Status: %.3f ms/frame (%.1f FPS)", deltatime * 1000.0f, framerate);
    ImGui::PopStyleColor();
}

void StatsWindow::UpdateElement() {
    static bool hasCheckedInitialState = false;
    static bool hasAutoFinished = false;
    static float openTimestamp = 0.0f;
    
    float currentTime = ImGui::GetTime();

    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    auto gfxDebuggerWin = gui->GetGuiWindow("GfxDebuggerWindow"); 

    // Initial State Check
    if (!hasCheckedInitialState && currentTime > 0.1f) {
        if (IsVisible()) {
            hasAutoFinished = true; 
        }
        hasCheckedInitialState = true;
    }

    // Auto-Kick Logic
    if (!hasAutoFinished) {
        if (currentTime > 0.5f && !gStatsTimerStarted) {
            // 1. Pop open the Stats Window
            if (!IsVisible()) {
                ToggleVisibility();
            }

            openTimestamp = currentTime;
            gStatsTimerStarted = true;
        }

        // Keep open for 1.0s to ensure the hardware acceleration kicks in
        if (gStatsTimerStarted && (currentTime - openTimestamp > 1.0f)) {
            // 3. Cleanly close the Stats Window
            Hide();

            gStatsTimerStarted = false;
            hasAutoFinished = true;
        }
    }
}
} // namespace Ship
