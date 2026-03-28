#if PLATFORM_MAC

#include "Tools/MCPEditorInputTools.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>

#include <unistd.h>  // usleep

// Mac virtual key codes (from HIToolbox/Events.h — hardcoded to avoid Carbon dependency)
static const CGKeyCode kMacVK_Return    = 36;
static const CGKeyCode kMacVK_Escape    = 53;
static const CGKeyCode kMacVK_Tab       = 48;
static const CGKeyCode kMacVK_Delete    = 51;  // Backspace
static const CGKeyCode kMacVK_Command   = 55;
static const CGKeyCode kMacVK_ANSI_A    = 0;

static void MacSendVK(CGKeyCode VK)
{
	CGEventRef Down = CGEventCreateKeyboardEvent(NULL, VK, true);
	CGEventRef Up   = CGEventCreateKeyboardEvent(NULL, VK, false);
	CGEventPost(kCGHIDEventTap, Down);
	CGEventPost(kCGHIDEventTap, Up);
	CFRelease(Down);
	CFRelease(Up);
}

static void MacSendCmd(CGKeyCode VK)
{
	CGEventRef CmdDown = CGEventCreateKeyboardEvent(NULL, kMacVK_Command, true);
	CGEventRef KeyDown = CGEventCreateKeyboardEvent(NULL, VK, true);
	CGEventRef KeyUp   = CGEventCreateKeyboardEvent(NULL, VK, false);
	CGEventRef CmdUp   = CGEventCreateKeyboardEvent(NULL, kMacVK_Command, false);
	CGEventPost(kCGHIDEventTap, CmdDown);
	CGEventPost(kCGHIDEventTap, KeyDown);
	CGEventPost(kCGHIDEventTap, KeyUp);
	CGEventPost(kCGHIDEventTap, CmdUp);
	CFRelease(CmdDown); CFRelease(KeyDown); CFRelease(KeyUp); CFRelease(CmdUp);
}

FMCPToolResult FMCPEditorInputTools::EditorInput(const TSharedPtr<FJsonObject>& Params)
{
	FString Action;
	if (!Params->TryGetStringField(TEXT("action"), Action))
		return FMCPToolResult::Error(TEXT("Missing required param: action"));

	// Find the top-level Slate editor window
	NSWindow* EditorWindow = nil;
	if (FSlateApplication::IsInitialized())
	{
		TArray<TSharedRef<SWindow>> Windows;
		FSlateApplication::Get().GetAllVisibleWindowsOrdered(Windows);
		for (const TSharedRef<SWindow>& Win : Windows)
		{
			if (Win->GetType() == EWindowType::Normal)
			{
				NSWindow* W = (NSWindow*)Win->GetNativeWindow()->GetOSWindowHandle();
				if (W) { EditorWindow = W; break; }
			}
		}
	}
	if (!EditorWindow)
		return FMCPToolResult::Error(TEXT("Could not find editor window"));

	// Window frame in AppKit coords (origin = bottom-left of screen)
	// No makeKeyAndOrderFront needed — CGEventPost routes by position, not focus.
	NSRect Frame    = [EditorWindow frame];
	CGFloat ScreenH = [[NSScreen mainScreen] frame].size.height;

	// ── Click / Double-click ──────────────────────────────────────────────────
	if (Action == TEXT("click") || Action == TEXT("double_click"))
	{
		double NX = 0.5, NY = 0.5;
		if (Params->HasField(TEXT("x"))) NX = Params->GetNumberField(TEXT("x"));
		if (Params->HasField(TEXT("y"))) NY = Params->GetNumberField(TEXT("y"));
		NX = FMath::Clamp(NX, 0.0, 1.0);
		NY = FMath::Clamp(NY, 0.0, 1.0);

		// Convert normalized window coords → CG screen coords (top-left origin)
		// AppKit: origin is bottom-left; CG: origin is top-left
		CGFloat CGX = Frame.origin.x + NX * Frame.size.width;
		CGFloat CGY = (ScreenH - Frame.origin.y - Frame.size.height) + NY * Frame.size.height;
		CGPoint Pos = CGPointMake(CGX, CGY);

		int32 ClickCount = (Action == TEXT("double_click")) ? 2 : 1;
		for (int32 i = 0; i < ClickCount; i++)
		{
			CGEventRef Down = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, Pos, kCGMouseButtonLeft);
			CGEventRef Up   = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp,   Pos, kCGMouseButtonLeft);
			CGEventPost(kCGHIDEventTap, Down);
			CGEventPost(kCGHIDEventTap, Up);
			CFRelease(Down);
			CFRelease(Up);
			if (i < ClickCount - 1) usleep(50000);
		}

		return FMCPToolResult::Success(FString::Printf(
			TEXT("%s at normalized (%.3f, %.3f)"), *Action, NX, NY));
	}

	// ── Type ─────────────────────────────────────────────────────────────────
	if (Action == TEXT("type"))
	{
		FString Text;
		if (!Params->TryGetStringField(TEXT("text"), Text))
			return FMCPToolResult::Error(TEXT("Missing 'text' param for type action"));

		usleep(50000);
		for (TCHAR Ch : Text)
		{
			UniChar UC = (UniChar)Ch;
			CGEventRef Down = CGEventCreateKeyboardEvent(NULL, 0, true);
			CGEventRef Up   = CGEventCreateKeyboardEvent(NULL, 0, false);
			CGEventKeyboardSetUnicodeString(Down, 1, &UC);
			CGEventKeyboardSetUnicodeString(Up,   1, &UC);
			CGEventPost(kCGHIDEventTap, Down);
			usleep(20000);
			CGEventPost(kCGHIDEventTap, Up);
			CFRelease(Down);
			CFRelease(Up);
			usleep(20000);
		}
		return FMCPToolResult::Success(FString::Printf(TEXT("Typed %d characters: \"%s\""), Text.Len(), *Text));
	}

	// ── Special keys ─────────────────────────────────────────────────────────
	if (Action == TEXT("key"))
	{
		FString Key;
		if (!Params->TryGetStringField(TEXT("key"), Key))
			return FMCPToolResult::Error(TEXT("Missing 'key' param for key action"));

		usleep(50000);

		if      (Key == TEXT("Enter"))     { MacSendVK(kMacVK_Return);  return FMCPToolResult::Success(TEXT("Pressed Enter")); }
		else if (Key == TEXT("Escape"))    { MacSendVK(kMacVK_Escape);  return FMCPToolResult::Success(TEXT("Pressed Escape")); }
		else if (Key == TEXT("Tab"))       { MacSendVK(kMacVK_Tab);     return FMCPToolResult::Success(TEXT("Pressed Tab")); }
		else if (Key == TEXT("Backspace")) { MacSendVK(kMacVK_Delete);  return FMCPToolResult::Success(TEXT("Pressed Backspace")); }
		else if (Key == TEXT("SelectAll")) { MacSendCmd(kMacVK_ANSI_A); return FMCPToolResult::Success(TEXT("Sent Cmd+A")); }
		else if (Key == TEXT("BackspaceAll"))
		{
			MacSendCmd(kMacVK_ANSI_A);
			usleep(50000);
			MacSendVK(kMacVK_Delete);
			return FMCPToolResult::Success(TEXT("Cleared field (Cmd+A, Backspace)"));
		}

		return FMCPToolResult::Error(FString::Printf(
			TEXT("Unknown key '%s'. Valid: Enter, Escape, Tab, Backspace, SelectAll, BackspaceAll"), *Key));
	}

	return FMCPToolResult::Error(FString::Printf(
		TEXT("Unknown action '%s'. Valid: click, double_click, type, key"), *Action));
}

#endif // PLATFORM_MAC
