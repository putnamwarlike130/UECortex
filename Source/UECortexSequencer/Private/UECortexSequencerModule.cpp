#include "UECortexSequencerModule.h"
#include "MCPSequencerTools.h"
#include "MCPToolRegistry.h"
#include "Modules/ModuleManager.h"

void FUECortexSequencerModule::StartupModule()
{
	FMCPToolRegistry::Get().RegisterModule(MakeShared<FMCPSequencerTools>());
}

void FUECortexSequencerModule::ShutdownModule()
{
	FMCPToolRegistry::Get().UnregisterModule(TEXT("sequencer"));
}

IMPLEMENT_MODULE(FUECortexSequencerModule, UECortexSequencer)
