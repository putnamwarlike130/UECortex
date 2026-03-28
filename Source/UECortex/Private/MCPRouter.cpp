#include "MCPRouter.h"
#include "MCPToolRegistry.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FString FMCPRouter::HandleRequest(const FString& Body)
{
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return MakeError(-1, -32700, TEXT("Parse error"));
	}

	int32 Id = 0;
	if (Root->HasField(TEXT("id")))
	{
		Id = (int32)Root->GetNumberField(TEXT("id"));
	}

	FString Method;
	if (!Root->TryGetStringField(TEXT("method"), Method))
	{
		return MakeError(Id, -32600, TEXT("Invalid Request: missing method"));
	}

	const TSharedPtr<FJsonObject>* ParamsPtr = nullptr;
	TSharedPtr<FJsonObject> Params;
	if (Root->TryGetObjectField(TEXT("params"), ParamsPtr) && ParamsPtr)
	{
		Params = *ParamsPtr;
	}
	else
	{
		Params = MakeShared<FJsonObject>();
	}

	if (Method == TEXT("initialize"))
	{
		return HandleInitialize(Id, Params);
	}
	else if (Method == TEXT("notifications/initialized"))
	{
		// Client ACK — no response needed for notifications
		return TEXT("");
	}
	else if (Method == TEXT("tools/list"))
	{
		return HandleToolsList(Id);
	}
	else if (Method == TEXT("tools/call"))
	{
		return HandleToolsCall(Id, Params);
	}
	else if (Method == TEXT("ping"))
	{
		auto Result = MakeShared<FJsonObject>();
		return MakeResult(Id, Result);
	}

	return MakeError(Id, -32601, FString::Printf(TEXT("Method not found: %s"), *Method));
}

FString FMCPRouter::HandleInitialize(int32 Id, const TSharedPtr<FJsonObject>& Params)
{
	auto Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("protocolVersion"), TEXT("2024-11-05"));

	auto Caps = MakeShared<FJsonObject>();
	auto ToolsCap = MakeShared<FJsonObject>();
	Caps->SetObjectField(TEXT("tools"), ToolsCap);
	Result->SetObjectField(TEXT("capabilities"), Caps);

	auto ServerInfo = MakeShared<FJsonObject>();
	ServerInfo->SetStringField(TEXT("name"), TEXT("UECortex"));
	ServerInfo->SetStringField(TEXT("version"), TEXT("1.0.0"));
	Result->SetObjectField(TEXT("serverInfo"), ServerInfo);

	return MakeResult(Id, Result);
}

FString FMCPRouter::HandleToolsList(int32 Id)
{
	auto Result = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> ToolsArray;

	for (TSharedPtr<FJsonObject> ToolObj : FMCPToolRegistry::Get().BuildToolsList())
	{
		ToolsArray.Add(MakeShared<FJsonValueObject>(ToolObj));
	}

	Result->SetArrayField(TEXT("tools"), ToolsArray);
	return MakeResult(Id, Result);
}

FString FMCPRouter::HandleToolsCall(int32 Id, const TSharedPtr<FJsonObject>& Params)
{
	FString ToolName;
	if (!Params->TryGetStringField(TEXT("name"), ToolName))
	{
		return MakeError(Id, -32602, TEXT("Invalid params: missing 'name'"));
	}

	const TSharedPtr<FJsonObject>* ArgsPtr = nullptr;
	TSharedPtr<FJsonObject> Args;
	if (Params->TryGetObjectField(TEXT("arguments"), ArgsPtr) && ArgsPtr)
	{
		Args = *ArgsPtr;
	}
	else
	{
		Args = MakeShared<FJsonObject>();
	}

	FMCPToolResult ToolResult = FMCPToolRegistry::Get().CallTool(ToolName, Args);

	if (!ToolResult.bSuccess)
	{
		return MakeError(Id, -32603, ToolResult.Message);
	}

	// Build MCP content response
	auto Result = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Content;

	if (ToolResult.bIsImage)
	{
		// Image content block per MCP spec
		auto ImageContent = MakeShared<FJsonObject>();
		ImageContent->SetStringField(TEXT("type"), TEXT("image"));
		ImageContent->SetStringField(TEXT("data"), ToolResult.ImageBase64);
		ImageContent->SetStringField(TEXT("mimeType"), ToolResult.ImageMimeType);
		Content.Add(MakeShared<FJsonValueObject>(ImageContent));
	}
	else
	{
		auto TextContent = MakeShared<FJsonObject>();
		TextContent->SetStringField(TEXT("type"), TEXT("text"));

		if (ToolResult.Data.IsValid())
		{
			FString DataStr;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&DataStr);
			FJsonSerializer::Serialize(ToolResult.Data.ToSharedRef(), Writer);
			TextContent->SetStringField(TEXT("text"), ToolResult.Message + TEXT("\n") + DataStr);
		}
		else
		{
			TextContent->SetStringField(TEXT("text"), ToolResult.Message);
		}

		Content.Add(MakeShared<FJsonValueObject>(TextContent));
	}

	Result->SetArrayField(TEXT("content"), Content);
	Result->SetBoolField(TEXT("isError"), false);

	return MakeResult(Id, Result);
}

FString FMCPRouter::MakeError(int32 Id, int32 Code, const FString& Message)
{
	auto Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("jsonrpc"), TEXT("2.0"));
	Root->SetNumberField(TEXT("id"), Id);

	auto Error = MakeShared<FJsonObject>();
	Error->SetNumberField(TEXT("code"), Code);
	Error->SetStringField(TEXT("message"), Message);
	Root->SetObjectField(TEXT("error"), Error);

	FString Out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return Out;
}

FString FMCPRouter::MakeResult(int32 Id, TSharedPtr<FJsonObject> ResultObj)
{
	auto Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("jsonrpc"), TEXT("2.0"));
	Root->SetNumberField(TEXT("id"), Id);
	Root->SetObjectField(TEXT("result"), ResultObj);

	FString Out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return Out;
}
