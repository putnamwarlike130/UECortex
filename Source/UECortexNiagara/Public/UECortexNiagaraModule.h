#pragma once
#include "Modules/ModuleInterface.h"

class FUECortexNiagaraModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
