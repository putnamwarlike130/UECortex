#pragma once

#include "CoreMinimal.h"
#include "MCPToolBase.h"

class UECORTEX_API FMCPSplineTools : public FMCPToolModuleBase
{
public:
	virtual FString GetModuleName() const override { return TEXT("spline"); }
	virtual void RegisterTools(TArray<FMCPToolDef>& OutTools) override;

private:
	static FMCPToolResult SplineCreate(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult SplineAddPoint(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult SplineModifyPoint(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult SplineAttachMesh(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult SplineQuery(const TSharedPtr<FJsonObject>& Args);
};
