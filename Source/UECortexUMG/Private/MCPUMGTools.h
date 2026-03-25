#pragma once
#include "MCPToolBase.h"

class FMCPUMGTools : public FMCPToolModuleBase
{
public:
	virtual FString GetModuleName() const override { return TEXT("umg"); }
	virtual void RegisterTools(TArray<FMCPToolDef>& OutTools) override;

private:
	static FMCPToolResult UMGListWidgets(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult UMGCreateWidget(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult UMGGetWidgetTree(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult UMGAddWidget(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult UMGSetText(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult UMGSetCanvasSlot(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult UMGSetAnchor(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult UMGSetVisibility(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult UMGAddVariable(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult UMGCompileWidget(const TSharedPtr<FJsonObject>& Params);
};
