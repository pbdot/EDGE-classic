#include "smc_imgui.h"

namespace smc
{

static void ShowFileMenu()
{
    if (ImGui::MenuItem("New Project"))
    {
    }

    if (ImGui::MenuItem("Manage Project"))
    {
    }

    ImGui::Separator();

    // TODO: see smc_ui_menus.cc given and recent files

    if (ImGui::MenuItem("Open Map"))
    {
    }

    if (ImGui::MenuItem("Save Map"))
    {
    }

    if (ImGui::MenuItem("Export Map"))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Fresh Map"))
    {
    }

    if (ImGui::MenuItem("Copy Map"))
    {
    }

    if (ImGui::MenuItem("Rename Map"))
    {
    }

    if (ImGui::MenuItem("Delete Map"))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Quit"))
    {
    }
}

static void ShowEditMenu()
{
    if (ImGui::MenuItem("Undo"))
    {
    }

    if (ImGui::MenuItem("Redo"))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Cut"))
    {
    }

    if (ImGui::MenuItem("Copy"))
    {
    }

    if (ImGui::MenuItem("Paste"))
    {
    }

    if (ImGui::MenuItem("Delete"))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Select All"))
    {
    }

    if (ImGui::MenuItem("Deselect All"))
    {
    }

    if (ImGui::MenuItem("Invert Selection"))
    {
    }

    if (ImGui::MenuItem("Last Selection"))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Operation Menu"))
    {
    }

    if (ImGui::MenuItem("Move Objects..."))
    {
    }

    if (ImGui::MenuItem("Scale Objects..."))
    {
    }

    if (ImGui::MenuItem("Rotate Objects..."))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Mirror Horizontally"))
    {
    }

    if (ImGui::MenuItem("Mirror &Vertically"))
    {
    }
}

static void ShowViewMenu()
{
    if (ImGui::MenuItem("Toggle Sprites"))
    {
    }

    if (ImGui::MenuItem("Toggle Gamma"))
    {
    }

    if (ImGui::MenuItem("Toggle Object Numbers"))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Zoom In"))
    {
    }

    if (ImGui::MenuItem("Zoom Out"))
    {
    }

    if (ImGui::MenuItem("Whole Map"))
    {
    }

    if (ImGui::MenuItem("Whole Selection"))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Default Props"))
    {
    }

    if (ImGui::MenuItem("Toggle 3D View"))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Find / Replace"))
    {
    }

    if (ImGui::MenuItem("Find Next"))
    {
    }

    if (ImGui::MenuItem("Go to Camera"))
    {
    }

    if (ImGui::MenuItem("Jump to Object"))
    {
    }
}

static void ShowBrowserMenu()
{
    if (ImGui::MenuItem("Textures"))
    {
    }

    if (ImGui::MenuItem("Flats"))
    {
    }

    if (ImGui::MenuItem("Things"))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Line Specials"))
    {
    }

    if (ImGui::MenuItem("Sector Types"))
    {
    }

    if (ImGui::MenuItem("General Types"))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Recent Textures"))
    {
    }

    if (ImGui::MenuItem("Recent Flats"))
    {
    }

    if (ImGui::MenuItem("Recent Things"))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Hide"))
    {
    }
}

static void ShowCheckMenu()
{
    if (ImGui::MenuItem("ALL"))
    {
    }

    if (ImGui::MenuItem("Major Stuff"))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Vertices"))
    {
    }

    if (ImGui::MenuItem("Sectors"))
    {
    }

    if (ImGui::MenuItem("LineDefs"))
    {
    }

    if (ImGui::MenuItem("Things"))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Textures"))
    {
    }

    if (ImGui::MenuItem("Tags"))
    {
    }
}

static void ShowToolsMenu()
{
    if (ImGui::MenuItem("Preferences"))
    {
    }

    if (ImGui::MenuItem("View Logs"))
    {
    }

    if (ImGui::MenuItem("Recalc Sectors"))
    {
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Test in Game"))
    {
    }

    if (ImGui::MenuItem("Build All Nodes"))
    {
    }

    if (ImGui::MenuItem("Edit Text Lump"))
    {
    }

    if (ImGui::MenuItem("Add BEHAVIOR Lump"))
    {
    }
}

static void ShowHelpMenu()
{
    if (ImGui::MenuItem("Online Docs..."))
    {
    }

    if (ImGui::MenuItem("About SnapMap-Classic"))
    {
    }
}

void SMC_ImGui_MainMenu()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ShowFileMenu();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            ShowEditMenu();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ShowViewMenu();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Browser"))
        {
            ShowBrowserMenu();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Check"))
        {
            ShowCheckMenu();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools"))
        {
            ShowToolsMenu();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            ShowHelpMenu();
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

} // namespace smc