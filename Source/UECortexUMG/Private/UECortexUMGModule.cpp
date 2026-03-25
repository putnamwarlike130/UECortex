#include "UECortexUMGModule.h"
#include "MCPUMGTools.h"
#include "MCPToolRegistry.h"
#include "Modules/ModuleManager.h"

void FUECortexUMGModule::StartupModule()
{
	FMCPToolRegistry::Get().RegisterModule(MakeShared<FMCPUMGTools>());
}

void FUECortexUMGModule::ShutdownModule()
{
	FMCPToolRegistry::Get().UnregisterModule(TEXT("umg"));
}

IMPLEMENT_MODULE(FUECortexUMGModule, UECortexUMG)
