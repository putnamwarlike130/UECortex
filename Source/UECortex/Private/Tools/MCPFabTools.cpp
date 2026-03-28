#include "Tools/MCPFabTools.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"

void FMCPFabTools::RegisterTools(TArray<FMCPToolDef>& OutTools)
{
	FMCPToolDef T;
	T.Name        = TEXT("fab_login_status");
	T.Description = TEXT(
		"Check whether the user is logged into their Epic Games / EOS account in the editor. "
		"Required for Fab asset purchases and downloads. Browse/search works without login. "
		"If not logged in, sign in via Edit → Sign In to Epic Games in the UE editor."
	);
	T.Handler = [](const TSharedPtr<FJsonObject>& P) { return FabLoginStatus(P); };
	OutTools.Add(T);
}

FMCPToolResult FMCPFabTools::FabLoginStatus(const TSharedPtr<FJsonObject>& Params)
{
	// Try EOS first, then fall back to the default online subsystem
	static const TCHAR* SubsystemsToCheck[] = { TEXT("EOS"), TEXT("EOS_PLUS"), nullptr };

	IOnlineSubsystem* OSS = nullptr;
	FString           OSSName;

	for (int32 i = 0; SubsystemsToCheck[i]; ++i)
	{
		OSS = IOnlineSubsystem::Get(SubsystemsToCheck[i]);
		if (OSS)
		{
			OSSName = SubsystemsToCheck[i];
			break;
		}
	}

	if (!OSS)
	{
		OSS     = IOnlineSubsystem::Get();   // default subsystem
		OSSName = OSS ? OSS->GetSubsystemName().ToString() : TEXT("None");
	}

	if (!OSS)
	{
		return FMCPToolResult::Success(
			TEXT("status: unavailable\n")
			TEXT("reason: No Online Subsystem found — EOS plugin may not be active.\n")
			TEXT("action: Ensure the OnlineSubsystemEOS plugin is enabled in your project.")
		);
	}

	IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		return FMCPToolResult::Success(FString::Printf(
			TEXT("status: unavailable\n")
			TEXT("subsystem: %s\n")
			TEXT("reason: Identity interface not available on this subsystem."),
			*OSSName
		));
	}

	const int32 LocalUserNum = 0;
	ELoginStatus::Type LoginStatus = Identity->GetLoginStatus(LocalUserNum);

	FString StatusStr;
	FString ActionStr;

	switch (LoginStatus)
	{
		case ELoginStatus::LoggedIn:
		{
			StatusStr = TEXT("logged_in");
			TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalUserNum);
			FString DisplayName = Identity->GetPlayerNickname(LocalUserNum);
			ActionStr = FString::Printf(
				TEXT("display_name: %s\nuser_id: %s"),
				*DisplayName,
				UserId.IsValid() ? *UserId->ToString() : TEXT("unknown")
			);
			break;
		}
		case ELoginStatus::UsingLocalProfile:
			StatusStr = TEXT("local_profile");
			ActionStr = TEXT("action: Logged in with a local profile only — sign in to Epic Games for Fab purchases.\n")
			            TEXT("        Edit → Sign In to Epic Games");
			break;
		case ELoginStatus::NotLoggedIn:
		default:
			StatusStr = TEXT("not_logged_in");
			ActionStr = TEXT("action: Sign in via Edit → Sign In to Epic Games in the UE editor.");
			break;
	}

	return FMCPToolResult::Success(FString::Printf(
		TEXT("status: %s\nsubsystem: %s\n%s"),
		*StatusStr, *OSSName, *ActionStr
	));
}
