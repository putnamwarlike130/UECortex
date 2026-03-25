#pragma once

#include "CoreMinimal.h"
#include "MCPToolBase.h"

class UECORTEX_API FMCPRenderingTools : public FMCPToolModuleBase
{
public:
	virtual FString GetModuleName() const override { return TEXT("rendering"); }
	virtual void RegisterTools(TArray<FMCPToolDef>& OutTools) override;

private:
	// Nanite
	static FMCPToolResult RenderSetNanite(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult RenderGetNaniteSettings(const TSharedPtr<FJsonObject>& Args);

	// Lumen
	static FMCPToolResult RenderSetLumen(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult RenderGetLumenSettings(const TSharedPtr<FJsonObject>& Args);

	// Materials
	static FMCPToolResult RenderSetMaterial(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult RenderGetMaterials(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult RenderCreateMaterialInstance(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult RenderSetMaterialParam(const TSharedPtr<FJsonObject>& Args);

	// Post Process
	static FMCPToolResult RenderSetPostProcess(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult RenderGetPostProcess(const TSharedPtr<FJsonObject>& Args);

	// Lighting
	static FMCPToolResult RenderSetSkyLight(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult RenderSetFog(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult RenderSetSkyAtmosphere(const TSharedPtr<FJsonObject>& Args);

	// Shadows & quality
	static FMCPToolResult RenderSetShadows(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult RenderSetAmbientOcclusion(const TSharedPtr<FJsonObject>& Args);
};
