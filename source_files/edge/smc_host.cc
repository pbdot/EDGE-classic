

namespace smc
{
    int SMC_Main(int argc, char *argv[]);
}


void SMC_Host_Init(int argc, char *argv[])
{
    smc::SMC_Main(argc, argv);
}