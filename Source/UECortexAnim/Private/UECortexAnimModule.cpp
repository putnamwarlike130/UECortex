#include "UECortexAnimModule.h"
#include "MCPAnimTools.h"
#include "MCPToolRegistry.h"
#include "Modules/ModuleManager.h"

void FUECortexAnimModule::StartupModule()
{
	FMCPToolRegistry::Get().RegisterModule(MakeShared<FMCPAnimTools>());
}

void FUECortexAnimModule::ShutdownModule()
{
	FMCPToolRegistry::Get().UnregisterModule(TEXT("anim"));
}

IMPLEMENT_MODULE(FUECortexAnimModule, UECortexAnim)
