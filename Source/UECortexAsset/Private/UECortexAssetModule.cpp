#include "UECortexAssetModule.h"
#include "MCPAssetTools.h"
#include "MCPToolRegistry.h"
#include "Modules/ModuleManager.h"

void FUECortexAssetModule::StartupModule()
{
	FMCPToolRegistry::Get().RegisterModule(MakeShared<FMCPAssetTools>());
}

void FUECortexAssetModule::ShutdownModule()
{
	FMCPToolRegistry::Get().UnregisterModule(TEXT("asset"));
}

IMPLEMENT_MODULE(FUECortexAssetModule, UECortexAsset)
