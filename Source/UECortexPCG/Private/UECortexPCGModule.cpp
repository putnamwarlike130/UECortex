#include "UECortexPCGModule.h"
#include "MCPPCGTools.h"
#include "MCPToolRegistry.h"
#include "Modules/ModuleManager.h"

void FUECortexPCGModule::StartupModule()
{
	FMCPToolRegistry::Get().RegisterModule(MakeShared<FMCPPCGTools>());
}

void FUECortexPCGModule::ShutdownModule()
{
	FMCPToolRegistry::Get().UnregisterModule(TEXT("pcg"));
}

IMPLEMENT_MODULE(FUECortexPCGModule, UECortexPCG)
