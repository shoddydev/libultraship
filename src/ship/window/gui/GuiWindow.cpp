#include "ship/window/gui/GuiWindow.h"
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/window/Window.h"
#include "ship/window/gui/Gui.h"

namespace Ship {

// Pulls the flag from StatsWindow.cpp so it can look inside that 1-second burst
extern bool gStatsTimerStarted;

GuiWindow::GuiWindow(const std::string& consoleVariable, bool isVisible, const std::string& name, ImVec2 originalSize,
                     uint32_t windowFlags)
    : GuiElement(isVisible), mName(name), mVisibilityConsoleVariable(consoleVariable), mOriginalSize(originalSize),
      mWindowFlags(windowFlags) {
    if (!mVisibilityConsoleVariable.empty()) {
        mIsVisible = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(mVisibilityConsoleVariable.c_str(),
                                                                                     mIsVisible);
        SyncVisibilityConsoleVariable();
    }
}

GuiWindow::GuiWindow(const std::string& consoleVariable, bool isVisible, const std::string& name, ImVec2 originalSize)
    : GuiWindow(consoleVariable, isVisible, name, originalSize, ImGuiWindowFlags_None) {
}

GuiWindow::GuiWindow(const std::string& consoleVariable, bool isVisible, const std::string& name)
    : GuiWindow(consoleVariable, isVisible, name, ImVec2{ -1, -1 }, ImGuiWindowFlags_None) {
}

GuiWindow::GuiWindow(const std::string& consoleVariable, const std::string& name, ImVec2 originalSize,
                     uint32_t windowFlags)
    : GuiWindow(consoleVariable, false, name, originalSize, windowFlags) {
}

GuiWindow::GuiWindow(const std::string& consoleVariable, const std::string& name, ImVec2 originalSize)
    : GuiWindow(consoleVariable, false, name, originalSize, ImGuiWindowFlags_None) {
}

GuiWindow::GuiWindow(const std::string& consoleVariable, const std::string& name)
    : GuiWindow(consoleVariable, false, name, ImVec2{ -1, -1 }, ImGuiWindowFlags_None) {
}

void GuiWindow::SetVisibility(bool visible) {
    mIsVisible = visible;
    SyncVisibilityConsoleVariable();
}

void GuiWindow::SyncVisibilityConsoleVariable() {
    if (mVisibilityConsoleVariable.empty()) {
        return;
    }

    bool shouldSave = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(
                          mVisibilityConsoleVariable.c_str(), 0) != IsVisible();

    if (IsVisible()) {
        Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(mVisibilityConsoleVariable.c_str(),
                                                                        IsVisible());
    } else {
        Ship::Context::GetInstance()->GetConsoleVariables()->ClearVariable(mVisibilityConsoleVariable.c_str());
    }

    if (shouldSave) {
        Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
}

void GuiWindow::Draw() {
    // Exit immediately if the window is closed to keep ImGui stack metrics pristine
    if (!IsVisible()) {
        return;
    }

    ImGuiWindowFlags flags = mWindowFlags;

    // Blank out the window layout ONLY during the 1-second pop sequence
    bool shouldBlankOut = gStatsTimerStarted && (mName == "Stats");

    if (shouldBlankOut) {
        // Drop title bars, backgrounds, borders, and grips into pure transparency
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ResizeGrip, ImVec4(0, 0, 0, 0));
        
        // Blank out text inside DrawElement completely
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 0));

        // Kill the structural line footprint
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    }

    if (mOriginalSize != ImVec2{ -1, -1 }) {
        ImGui::SetNextWindowSize(mOriginalSize, ImGuiCond_FirstUseEver);
    }

    if (!ImGui::Begin(mName.c_str(), &mIsVisible, flags)) {
        ImGui::End();
    } else {
        DrawElement(); 
        ImGui::End();
    }

    if (shouldBlankOut) {
        // Balanced stack unwinding
        ImGui::PopStyleColor(5); 
        ImGui::PopStyleVar();
    }

    SyncVisibilityConsoleVariable();
}

std::string GuiWindow::GetName() {
    return mName;
}

void GuiWindow::BeginGroupPanel(const char* name, const ImVec2& size) {
    ImGui::BeginGroup();

    auto itemSpacing = ImGui::GetStyle().ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();
    ImGui::BeginGroup();

    ImVec2 effectiveSize = size;
    if (size.x < 0.0f) {
        effectiveSize.x = ImGui::GetContentRegionAvail().x;
    } else {
        effectiveSize.x = size.x;
    }
    ImGui::Dummy(ImVec2(effectiveSize.x, 0.0f));

    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::TextUnformatted(name);
    auto labelMin = ImGui::GetItemRectMin();
    auto labelMax = ImGui::GetItemRectMax();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(0.0, frameHeight + itemSpacing.y));
    ImGui::BeginGroup();

    ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->WorkRect.Max.x -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->InnerRect.Max.x -= frameHeight * 0.5f;
#else
    ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x -= frameHeight * 0.5f;
#endif
    ImGui::GetCurrentWindow()->Size.x -= frameHeight;

    auto itemWidth = ImGui::CalcItemWidth();
    ImGui::PushItemWidth(ImMax(0.0f, itemWidth - frameHeight));
    mGroupPanelLabelStack.push_back(ImRect(labelMin, labelMax));
}

void GuiWindow::EndGroupPanel(float minHeight) {
    ImGui::PopItemWidth();

    auto itemSpacing = ImGui::GetStyle().ItemSpacing;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();

    ImGui::EndGroup();

    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::Dummy(ImVec2(0.0, std::max(frameHeight - frameHeight * 0.5f - itemSpacing.y, minHeight)));

    ImGui::EndGroup();

    auto itemMin = ImGui::GetItemRectMin();
    auto itemMax = ImGui::GetItemRectMax();

    auto labelRect = mGroupPanelLabelStack.back();
    mGroupPanelLabelStack.pop_back();

    ImVec2 halfFrame = ImVec2(frameHeight * 0.25f, frameHeight) * 0.5f;
    ImRect frameRect = ImRect(itemMin + halfFrame, itemMax - ImVec2(halfFrame.x, 0.0f));
    labelRect.Min.x -= itemSpacing.x;
    labelRect.Max.x += itemSpacing.x;
    for (int i = 0; i < 4; ++i) {
        switch (i) {
            case 0:
                ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(labelRect.Min.x, FLT_MAX), true);
                break;
            case 1:
                ImGui::PushClipRect(ImVec2(labelRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX), true);
                break;
            case 2:
                ImGui::PushClipRect(ImVec2(labelRect.Min.x, -FLT_MAX), ImVec2(labelRect.Max.x, labelRect.Min.y), true);
                break;
            case 3:
                ImGui::PushClipRect(ImVec2(labelRect.Min.x, labelRect.Max.y), ImVec2(labelRect.Max.x, FLT_MAX), true);
                break;
        }

        ImGui::GetWindowDrawList()->AddRect(frameRect.Min, frameRect.Max,
                                            ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)), halfFrame.x);

        ImGui::PopClipRect();
    }

    ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->WorkRect.Max.x += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->InnerRect.Max.x += frameHeight * 0.5f;
#else
    ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x += frameHeight * 0.5f;
#endif
    ImGui::GetCurrentWindow()->Size.x += frameHeight;

    ImGui::Dummy(ImVec2(0.0f, 0.0f));

    ImGui::EndGroup();
}
} // namespace Ship
