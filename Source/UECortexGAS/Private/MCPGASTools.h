#pragma once

#include "CoreMinimal.h"
#include "MCPToolBase.h"

class FMCPGASTools : public FMCPToolModuleBase
{
public:
	virtual FString GetModuleName() const override { return TEXT("gas"); }
	virtual void RegisterTools(TArray<FMCPToolDef>& OutTools) override;

private:
	static FMCPToolResult GASAddAbilityComponent(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult GASGrantAbility(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult GASRevokeAbility(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult GASGetAbilities(const TSharedPtr<FJsonObject>& Args);

	static FMCPToolResult GASAddGameplayTag(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult GASRemoveGameplayTag(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult GASGetGameplayTags(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult GASListRegisteredTags(const TSharedPtr<FJsonObject>& Args);

	static FMCPToolResult GASSetAttribute(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult GASGetAttributes(const TSharedPtr<FJsonObject>& Args);

	static FMCPToolResult GASApplyEffect(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult GASGetActiveEffects(const TSharedPtr<FJsonObject>& Args);
};
