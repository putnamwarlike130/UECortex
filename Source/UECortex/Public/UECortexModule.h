#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

// Custom log category — visible in the UE Output Log by default
DECLARE_LOG_CATEGORY_EXTERN(LogUECortex, Log, All);

// Version shorthand macros
#define UE_MCP_VERSION_5_7_PLUS (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7)
#define UE_MCP_VERSION_5_6_ONLY (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 6)

class FMCPHttpServer;

class FUECortexModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FUECortexModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FUECortexModule>("UECortex");
	}

private:
	TUniquePtr<FMCPHttpServer> HttpServer;
};
