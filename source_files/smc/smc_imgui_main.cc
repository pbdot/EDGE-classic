#include "smc_imgui.h"

namespace smc
{

void SMC_ImGui_StartFrame()
{
}

void SMC_ImGui_FinishFrame()
{
    //ImGui::ShowDemoWindow();

    SMC_ImGui_MainMenu();
}

} // namespace smc