#pragma once

#include "CoreMinimal.h"
#include "MCPToolBase.h"

class UECORTEX_API FMCPVolumeTools : public FMCPToolModuleBase
{
public:
	virtual FString GetModuleName() const override { return TEXT("volume"); }
	virtual void RegisterTools(TArray<FMCPToolDef>& OutTools) override;

private:
	static FMCPToolResult VolumeCreate(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult VolumeSetProperties(const TSharedPtr<FJsonObject>& Args);
};
