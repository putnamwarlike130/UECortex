#pragma once
#include "MCPToolBase.h"

class FMCPPythonTools : public FMCPToolModuleBase
{
public:
	virtual FString GetModuleName() const override { return TEXT("python"); }
	virtual void RegisterTools(TArray<FMCPToolDef>& OutTools) override;

private:
	static FMCPToolResult PythonExec(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult PythonExecFile(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult PythonCheck(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult PythonSetVar(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult PythonGetVar(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult PythonListScripts(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult PythonRegisterTool(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult PythonUnregisterTool(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult PythonListDynamicTools(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult PythonReloadToolsFromFile(const TSharedPtr<FJsonObject>& Params);
};
