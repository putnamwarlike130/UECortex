#include "MCPToolRegistry.h"
#include "UECortexModule.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FMCPToolRegistry& FMCPToolRegistry::Get()
{
	static FMCPToolRegistry Instance;
	return Instance;
}

void FMCPToolRegistry::RegisterModule(TSharedRef<FMCPToolModuleBase> Module)
{
	Module->RegisterTools(Tools);
	Modules.Add(Module);
	UE_LOG(LogUECortex, Log, TEXT("UECortex: Registered module '%s'"), *Module->GetModuleName());
}

void FMCPToolRegistry::UnregisterModule(const FString& ModuleName)
{
	Modules.RemoveAll([&](const TSharedRef<FMCPToolModuleBase>& M) {
		return M->GetModuleName() == ModuleName;
	});
	const FString Prefix = ModuleName + TEXT("_");
	Tools.RemoveAll([&](const FMCPToolDef& T) {
		return T.Name.StartsWith(Prefix);
	});
	UE_LOG(LogUECortex, Log, TEXT("UECortex: Unregistered module '%s'"), *ModuleName);
}

void FMCPToolRegistry::RegisterTool(const FMCPToolDef& Tool)
{
	// Replace if already registered
	for (FMCPToolDef& Existing : Tools)
	{
		if (Existing.Name == Tool.Name)
		{
			Existing = Tool;
			UE_LOG(LogUECortex, Log, TEXT("UECortex: Updated dynamic tool '%s'"), *Tool.Name);
			return;
		}
	}
	Tools.Add(Tool);
	UE_LOG(LogUECortex, Log, TEXT("UECortex: Registered dynamic tool '%s'"), *Tool.Name);
}

void FMCPToolRegistry::UnregisterTool(const FString& ToolName)
{
	int32 Removed = Tools.RemoveAll([&](const FMCPToolDef& T) { return T.Name == ToolName; });
	if (Removed > 0)
		UE_LOG(LogUECortex, Log, TEXT("UECortex: Unregistered dynamic tool '%s'"), *ToolName);
}

FMCPToolResult FMCPToolRegistry::CallTool(const FString& ToolName, const TSharedPtr<FJsonObject>& Args)
{
	for (const FMCPToolDef& Tool : Tools)
	{
		if (Tool.Name == ToolName)
		{
			if (!Tool.bEnabled)
			{
				return FMCPToolResult::Error(FString::Printf(
					TEXT("Tool '%s' is currently disabled."), *ToolName));
			}
			return Tool.Handler(Args);
		}
	}
	return FMCPToolResult::Error(FString::Printf(TEXT("Unknown tool: %s"), *ToolName));
}

TArray<TSharedPtr<FJsonObject>> FMCPToolRegistry::BuildToolsList() const
{
	TArray<TSharedPtr<FJsonObject>> Result;

	for (const FMCPToolDef& Tool : Tools)
	{
		if (!Tool.bEnabled) continue;

		auto ToolObj = MakeShared<FJsonObject>();
		ToolObj->SetStringField(TEXT("name"), Tool.Name);
		ToolObj->SetStringField(TEXT("description"), Tool.Description);

		// Build inputSchema
		auto Schema = MakeShared<FJsonObject>();
		Schema->SetStringField(TEXT("type"), TEXT("object"));

		auto Properties = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> Required;

		for (const FMCPParamSchema& Param : Tool.Params)
		{
			auto PropObj = MakeShared<FJsonObject>();
			PropObj->SetStringField(TEXT("type"), Param.Type);
			PropObj->SetStringField(TEXT("description"), Param.Description);
			Properties->SetObjectField(Param.Name, PropObj);

			if (Param.bRequired)
			{
				Required.Add(MakeShared<FJsonValueString>(Param.Name));
			}
		}

		Schema->SetObjectField(TEXT("properties"), Properties);
		Schema->SetArrayField(TEXT("required"), Required);
		ToolObj->SetObjectField(TEXT("inputSchema"), Schema);

		Result.Add(ToolObj);
	}

	return Result;
}

bool FMCPToolRegistry::EnableTool(const FString& ToolName)
{
	for (FMCPToolDef& Tool : Tools)
	{
		if (Tool.Name == ToolName)
		{
			Tool.bEnabled = true;
			return true;
		}
	}
	return false;
}

bool FMCPToolRegistry::DisableTool(const FString& ToolName)
{
	for (FMCPToolDef& Tool : Tools)
	{
		if (Tool.Name == ToolName)
		{
			Tool.bEnabled = false;
			return true;
		}
	}
	return false;
}

bool FMCPToolRegistry::EnableCategory(const FString& CategoryName)
{
	// Category = module name prefix before underscore (e.g. "actor_spawn" -> "actor")
	bool bAny = false;
	for (FMCPToolDef& Tool : Tools)
	{
		FString Prefix;
		Tool.Name.Split(TEXT("_"), &Prefix, nullptr);
		if (Prefix.Equals(CategoryName, ESearchCase::IgnoreCase))
		{
			Tool.bEnabled = true;
			bAny = true;
		}
	}
	return bAny;
}

bool FMCPToolRegistry::DisableCategory(const FString& CategoryName)
{
	bool bAny = false;
	for (FMCPToolDef& Tool : Tools)
	{
		FString Prefix;
		Tool.Name.Split(TEXT("_"), &Prefix, nullptr);
		if (Prefix.Equals(CategoryName, ESearchCase::IgnoreCase))
		{
			Tool.bEnabled = false;
			bAny = true;
		}
	}
	return bAny;
}

void FMCPToolRegistry::ResetAll()
{
	for (FMCPToolDef& Tool : Tools)
	{
		Tool.bEnabled = true;
	}
}

int32 FMCPToolRegistry::GetEnabledToolCount() const
{
	int32 Count = 0;
	for (const FMCPToolDef& Tool : Tools)
	{
		if (Tool.bEnabled) Count++;
	}
	return Count;
}

void FMCPToolRegistry::GetModuleStatus(TArray<FString>& OutActive, TArray<FString>& OutInactive) const
{
	for (const TSharedRef<FMCPToolModuleBase>& Module : Modules)
	{
		OutActive.Add(Module->GetModuleName());
	}
}
