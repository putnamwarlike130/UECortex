#pragma once
#include "MCPToolBase.h"

class UECORTEX_API FMCPFabTools : public FMCPToolModuleBase
{
public:
	virtual FString GetModuleName() const override { return TEXT("fab"); }
	virtual void RegisterTools(TArray<FMCPToolDef>& OutTools) override;

private:
	static FMCPToolResult FabLoginStatus(const TSharedPtr<FJsonObject>& Params);
};
