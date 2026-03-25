#pragma once

#include "CoreMinimal.h"
#include "MCPToolBase.h"

class FMCPPCGTools : public FMCPToolModuleBase
{
public:
	virtual FString GetModuleName() const override { return TEXT("pcg"); }
	virtual void RegisterTools(TArray<FMCPToolDef>& OutTools) override;

private:
	static FMCPToolResult PCGCreateGraph(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult PCGAddNode(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult PCGConnectPins(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult PCGDisconnectPins(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult PCGSetNodeParam(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult PCGGetGraph(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult PCGCreateSubgraph(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult PCGPlaceVolume(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult PCGPlaceSeedActor(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult PCGExecuteGraph(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult PCGListNodeTypes(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult PCGGetNodeParams(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult PCGSetGraphParam(const TSharedPtr<FJsonObject>& Args);
	static FMCPToolResult PCGCreateAttributeNode(const TSharedPtr<FJsonObject>& Args);

	// Helpers
	static class UPCGGraph* LoadPCGGraph(const FString& Path);
	static class UPCGNode*  FindNodeByName(UPCGGraph* Graph, const FString& Name);
};
