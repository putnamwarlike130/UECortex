#pragma once
#include "Modules/ModuleInterface.h"

class FUECortexAnimModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
