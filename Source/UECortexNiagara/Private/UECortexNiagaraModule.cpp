#include "UECortexNiagaraModule.h"
#include "MCPNiagaraTools.h"
#include "MCPToolRegistry.h"
#include "Modules/ModuleManager.h"

void FUECortexNiagaraModule::StartupModule()
{
	FMCPToolRegistry::Get().RegisterModule(MakeShared<FMCPNiagaraTools>());
}

void FUECortexNiagaraModule::ShutdownModule()
{
	FMCPToolRegistry::Get().UnregisterModule(TEXT("niagara"));
}

IMPLEMENT_MODULE(FUECortexNiagaraModule, UECortexNiagara)
