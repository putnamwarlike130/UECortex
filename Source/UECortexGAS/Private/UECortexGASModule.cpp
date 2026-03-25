#include "UECortexGASModule.h"
#include "MCPGASTools.h"
#include "MCPToolRegistry.h"
#include "Modules/ModuleManager.h"

void FUECortexGASModule::StartupModule()
{
	FMCPToolRegistry::Get().RegisterModule(MakeShared<FMCPGASTools>());
}

void FUECortexGASModule::ShutdownModule()
{
	FMCPToolRegistry::Get().UnregisterModule(TEXT("gas"));
}

IMPLEMENT_MODULE(FUECortexGASModule, UECortexGAS)
