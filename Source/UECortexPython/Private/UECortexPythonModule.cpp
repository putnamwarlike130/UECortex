#include "UECortexPythonModule.h"
#include "MCPPythonTools.h"
#include "MCPToolRegistry.h"
#include "Modules/ModuleManager.h"

void FUECortexPythonModule::StartupModule()
{
	FMCPToolRegistry::Get().RegisterModule(MakeShared<FMCPPythonTools>());
}

void FUECortexPythonModule::ShutdownModule()
{
	FMCPToolRegistry::Get().UnregisterModule(TEXT("python"));
}

IMPLEMENT_MODULE(FUECortexPythonModule, UECortexPython)
