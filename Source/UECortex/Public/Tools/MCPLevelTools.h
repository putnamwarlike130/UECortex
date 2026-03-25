#pragma once

#include "CoreMinimal.h"
#include "MCPToolBase.h"

class UECORTEX_API FMCPLevelTools : public FMCPToolModuleBase
{
public:
	virtual FString GetModuleName() const override { return TEXT("level"); }
	virtual void RegisterTools(TArray<FMCPToolDef>& OutTools) override;

private:
	static FMCPToolResult ActorSpawn(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult ActorDelete(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult ActorSetTransform(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult ActorGetProperties(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult ActorList(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult ActorFindByName(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult ActorSpawnOnSurface(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult ActorDuplicate(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult ActorGetSelected(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult ActorSetProperty(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult LevelGetInfo(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult ViewportFocus(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult ViewportSetCamera(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult ConsoleCommand(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult LineTrace(const TSharedPtr<FJsonObject>& Args);
};
