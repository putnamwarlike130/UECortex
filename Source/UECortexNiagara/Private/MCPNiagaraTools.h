#pragma once

#include "CoreMinimal.h"
#include "MCPToolBase.h"

class FMCPNiagaraTools : public FMCPToolModuleBase
{
public:
	virtual FString GetModuleName() const override { return TEXT("niagara"); }
	virtual void RegisterTools(TArray<FMCPToolDef>& OutTools) override;

private:
	static FMCPToolResult NiagaraListSystems(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult NiagaraCreateSystem(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult NiagaraListEmitters(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult NiagaraGetParameters(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult NiagaraSetParameter(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult NiagaraSpawnComponent(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult NiagaraSetComponentParameter(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult NiagaraActivateComponent(const TSharedPtr<FJsonObject>& Args);
};
