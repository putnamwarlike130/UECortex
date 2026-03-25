#pragma once
#include "Modules/ModuleInterface.h"

class FUECortexPCGModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
