#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FMCPRouter
{
public:
	// Route a JSON-RPC request, return the JSON-RPC response as a string
	static FString HandleRequest(const FString& Body);

private:
	static FString HandleInitialize(int32 Id, const TSharedPtr<FJsonObject>& Params);
	static FString HandleToolsList(int32 Id);
	static FString HandleToolsCall(int32 Id, const TSharedPtr<FJsonObject>& Params);

	static FString MakeError(int32 Id, int32 Code, const FString& Message);
	static FString MakeResult(int32 Id, TSharedPtr<FJsonObject> Result);
};
