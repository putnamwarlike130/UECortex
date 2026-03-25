#pragma once
#include "MCPToolBase.h"

class FMCPSequencerTools : public FMCPToolModuleBase
{
public:
	virtual FString GetModuleName() const override { return TEXT("sequencer"); }
	virtual void RegisterTools(TArray<FMCPToolDef>& OutTools) override;

private:
	static FMCPToolResult SeqCreate(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult SeqList(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult SeqGetInfo(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult SeqSetPlaybackRange(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult SeqAddActorBinding(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult SeqAddTransformTrack(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult SeqAddKeyframe(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult SeqAddCameraCut(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult SeqAddSpawnableCamera(const TSharedPtr<FJsonObject>& Params);
	static FMCPToolResult SeqAddVisibilityTrack(const TSharedPtr<FJsonObject>& Params);
};
