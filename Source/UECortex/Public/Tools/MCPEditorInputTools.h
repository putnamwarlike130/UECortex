#pragma once
#include "MCPToolBase.h"

class UECORTEX_API FMCPEditorInputTools : public FMCPToolModuleBase
{
public:
	virtual FString GetModuleName() const override { return TEXT("editor_input"); }
	virtual void RegisterTools(TArray<FMCPToolDef>& OutTools) override;

private:
	static FMCPToolResult EditorInput(const TSharedPtr<FJsonObject>& Params);
};
