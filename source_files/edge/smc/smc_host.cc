
#include <epi.h>

#include "smc_public.h"

namespace edge
{
void SMC_Host_Initialize()
{
    if (smc::SMC_Main())
    {
        FatalError("SMC_Host_Initialize: Failed to initialize\n");
    }
}

void SMC_Host_Shutdown()
{
    smc::SMC_Shutdown();
}

} // namespace edge
