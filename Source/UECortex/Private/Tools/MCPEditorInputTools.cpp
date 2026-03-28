#include "Tools/MCPEditorInputTools.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

// ── Registration ──────────────────────────────────────────────────────────────

void FMCPEditorInputTools::RegisterTools(TArray<FMCPToolDef>& OutTools)
{
	FMCPToolDef T;
	T.Name = TEXT("editor_input");
	T.Description = TEXT(
		"Simulate mouse/keyboard input on the editor window (Windows only). "
		"action='click': left-click at normalized position x,y (0-1 relative to editor window). "
		"action='double_click': double left-click at x,y. "
		"action='type': type a string at current focus. "
		"action='key': press Enter, Escape, Tab, Backspace, SelectAll, or BackspaceAll."
	);
	T.Params = {
		{ TEXT("action"), TEXT("string"), TEXT("'click', 'double_click', 'type', or 'key'"), true },
		{ TEXT("x"),      TEXT("number"), TEXT("Normalized X (0-1). Required for click/double_click"), false },
		{ TEXT("y"),      TEXT("number"), TEXT("Normalized Y (0-1). Required for click/double_click"), false },
		{ TEXT("text"),   TEXT("string"), TEXT("Text to type. Required for 'type'"), false },
		{ TEXT("key"),    TEXT("string"), TEXT("Key name: Enter, Escape, Tab, Backspace, SelectAll, BackspaceAll"), false },
	};
	T.Handler = [](const TSharedPtr<FJsonObject>& P) { return EditorInput(P); };
	OutTools.Add(T);
}

// ── Implementation ────────────────────────────────────────────────────────────
// Mac implementation lives in MCPEditorInputTools.mm

#if !PLATFORM_MAC   // Mac defines EditorInput in its own .mm

FMCPToolResult FMCPEditorInputTools::EditorInput(const TSharedPtr<FJsonObject>& Params)
{
#if !PLATFORM_WINDOWS
	return FMCPToolResult::Error(TEXT("editor_input is only supported on Windows and macOS"));
#else
	FString Action;
	if (!Params->TryGetStringField(TEXT("action"), Action))
		return FMCPToolResult::Error(TEXT("Missing required param: action"));

	// Find the main editor window (for coordinate reference and fallback)
	HWND MainHwnd = nullptr;
	if (FSlateApplication::IsInitialized())
	{
		TArray<TSharedRef<SWindow>> Windows;
		FSlateApplication::Get().GetAllVisibleWindowsOrdered(Windows);
		for (const TSharedRef<SWindow>& Win : Windows)
		{
			if (Win->GetType() == EWindowType::Normal)
			{
				MainHwnd = (HWND)Win->GetNativeWindow()->GetOSWindowHandle();
				if (MainHwnd) break;
			}
		}
	}
	if (!MainHwnd)
		return FMCPToolResult::Error(TEXT("Could not find editor window"));

	RECT WinRect;
	GetWindowRect(MainHwnd, &WinRect);
	int32 WinW = WinRect.right  - WinRect.left;
	int32 WinH = WinRect.bottom - WinRect.top;

	// ── Click / Double-click ──────────────────────────────────────────────────
	if (Action == TEXT("click") || Action == TEXT("double_click"))
	{
		double NX = 0.5, NY = 0.5;
		if (Params->HasField(TEXT("x"))) NX = Params->GetNumberField(TEXT("x"));
		if (Params->HasField(TEXT("y"))) NY = Params->GetNumberField(TEXT("y"));
		NX = FMath::Clamp(NX, 0.0, 1.0);
		NY = FMath::Clamp(NY, 0.0, 1.0);

		// Convert normalized coords to absolute screen position
		int32 ScreenX = WinRect.left + (int32)(NX * WinW);
		int32 ScreenY = WinRect.top  + (int32)(NY * WinH);

		// WindowFromPoint: finds the actual HWND at this screen position,
		// correctly handles Fab browser (separate HWND on top of main editor)
		POINT ScreenPt = { ScreenX, ScreenY };
		HWND TargetHwnd = WindowFromPoint(ScreenPt);
		if (!TargetHwnd) TargetHwnd = MainHwnd;

		// Convert to client coordinates of the target window
		POINT ClientPt = ScreenPt;
		ScreenToClient(TargetHwnd, &ClientPt);
		LPARAM LParam = MAKELPARAM(ClientPt.x, ClientPt.y);

		int32 ClickCount = (Action == TEXT("double_click")) ? 2 : 1;
		for (int32 i = 0; i < ClickCount; i++)
		{
			PostMessageW(TargetHwnd, WM_LBUTTONDOWN, MK_LBUTTON, LParam);
			::Sleep(30);
			PostMessageW(TargetHwnd, WM_LBUTTONUP, 0, LParam);
			if (i < ClickCount - 1) ::Sleep(80);
		}

		return FMCPToolResult::Success(FString::Printf(
			TEXT("%s → HWND %lld client (%d,%d) screen (%d,%d)"),
			*Action, (int64)TargetHwnd, ClientPt.x, ClientPt.y, ScreenX, ScreenY));
	}

	// ── Type ─────────────────────────────────────────────────────────────────
	if (Action == TEXT("type"))
	{
		FString Text;
		if (!Params->TryGetStringField(TEXT("text"), Text))
			return FMCPToolResult::Error(TEXT("Missing 'text' param for type action"));

		// Post to the foreground window — after a click, the target window
		// (Fab browser or main editor) will be the foreground window.
		HWND TypeTarget = GetForegroundWindow();
		if (!TypeTarget) TypeTarget = MainHwnd;

		::Sleep(50);
		for (TCHAR Ch : Text)
		{
			PostMessageW(TypeTarget, WM_CHAR, (WPARAM)Ch, 1);
			::Sleep(30);
		}
		return FMCPToolResult::Success(FString::Printf(
			TEXT("Typed %d characters to HWND %lld: \"%s\""), Text.Len(), (int64)TypeTarget, *Text));
	}

	// ── Special keys ─────────────────────────────────────────────────────────
	if (Action == TEXT("key"))
	{
		FString Key;
		if (!Params->TryGetStringField(TEXT("key"), Key))
			return FMCPToolResult::Error(TEXT("Missing 'key' param for key action"));

		// Use foreground window so Enter/Escape go to the same window that received typing
		HWND KeyTarget = GetForegroundWindow();
		if (!KeyTarget) KeyTarget = MainHwnd;

		auto PostVK = [&KeyTarget](WORD VK)
		{
			PostMessageW(KeyTarget, WM_KEYDOWN, VK, 1);
			::Sleep(20);
			PostMessageW(KeyTarget, WM_KEYUP, VK, 1);
		};

		auto PostCtrl = [&KeyTarget, &PostVK](WORD VK)
		{
			PostMessageW(KeyTarget, WM_KEYDOWN, VK_CONTROL, 1);
			::Sleep(20);
			PostMessageW(KeyTarget, WM_KEYDOWN, VK, 1);
			::Sleep(20);
			PostMessageW(KeyTarget, WM_KEYUP, VK, 1);
			::Sleep(20);
			PostMessageW(KeyTarget, WM_KEYUP, VK_CONTROL, 1);
		};

		::Sleep(50);

		if      (Key == TEXT("Enter"))     { PostVK(VK_RETURN);  return FMCPToolResult::Success(TEXT("Pressed Enter")); }
		else if (Key == TEXT("Escape"))    { PostVK(VK_ESCAPE);  return FMCPToolResult::Success(TEXT("Pressed Escape")); }
		else if (Key == TEXT("Tab"))       { PostVK(VK_TAB);     return FMCPToolResult::Success(TEXT("Pressed Tab")); }
		else if (Key == TEXT("Backspace")) { PostVK(VK_BACK);    return FMCPToolResult::Success(TEXT("Pressed Backspace")); }
		else if (Key == TEXT("SelectAll")) { PostCtrl('A');       return FMCPToolResult::Success(TEXT("Sent Ctrl+A")); }
		else if (Key == TEXT("BackspaceAll"))
		{
			PostCtrl('A');
			::Sleep(50);
			PostVK(VK_BACK);
			return FMCPToolResult::Success(TEXT("Cleared field (Ctrl+A, Backspace)"));
		}

		return FMCPToolResult::Error(FString::Printf(
			TEXT("Unknown key '%s'. Valid: Enter, Escape, Tab, Backspace, SelectAll, BackspaceAll"), *Key));
	}

	return FMCPToolResult::Error(FString::Printf(
		TEXT("Unknown action '%s'. Valid: click, double_click, type, key"), *Action));
#endif // PLATFORM_WINDOWS
}

#endif // !PLATFORM_MAC
