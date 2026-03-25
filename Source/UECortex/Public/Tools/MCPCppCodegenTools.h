#pragma once

#include "CoreMinimal.h"
#include "MCPToolBase.h"

class UECORTEX_API FMCPCppCodegenTools : public FMCPToolModuleBase
{
public:
	virtual FString GetModuleName() const override { return TEXT("cpp_codegen"); }
	virtual void RegisterTools(TArray<FMCPToolDef>& OutTools) override;

private:
	static FMCPToolResult CppCreateActor(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult CppCreateComponent(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult CppCreateInterface(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult CppCreateGASAbility(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult CppCreateGASEffect(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult CppCreateAttributeSet(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult CppAddUProperty(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult CppAddUFunction(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult CppAddInclude(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult CppHotReload(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult CppGetClassInfo(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult CppRunUBT(const TSharedPtr<FJsonObject>& Args);

	// File helpers
	static FString GetSourceDir(const FString& SubDir = TEXT(""));
	static FString GetAPImacro();
	static bool    WriteFile(const FString& Path, const FString& Content, bool bForce, FString& OutError);
	static bool    ReadFile(const FString& Path, FString& OutContent, FString& OutError);
};
