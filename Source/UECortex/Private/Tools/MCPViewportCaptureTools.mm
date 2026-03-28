#if PLATFORM_MAC

#include "Tools/MCPViewportCaptureTools.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>

bool FMCPViewportCaptureTools::CaptureEditorWindow(TArray<FColor>& OutPixels, int32& OutW, int32& OutH)
{
	// Find the top-level Slate window and get its NSWindow handle
	NSWindow* TargetWindow = nil;
	if (FSlateApplication::IsInitialized())
	{
		TArray<TSharedRef<SWindow>> Windows;
		FSlateApplication::Get().GetAllVisibleWindowsOrdered(Windows);
		for (const TSharedRef<SWindow>& Win : Windows)
		{
			if (Win->GetType() == EWindowType::Normal)
			{
				NSWindow* W = (NSWindow*)Win->GetNativeWindow()->GetOSWindowHandle();
				if (W) { TargetWindow = W; break; }
			}
		}
	}
	if (!TargetWindow) return false;

	CGWindowID WindowID = (CGWindowID)[TargetWindow windowNumber];

	// Capture the window — kCGWindowImageBoundsIgnoreFraming excludes the title bar shadow
	CGImageRef Image = CGWindowListCreateImage(
		CGRectNull,
		kCGWindowListOptionIncludingWindow,
		WindowID,
		kCGWindowImageBoundsIgnoreFraming
	);
	if (!Image) return false;

	OutW = (int32)CGImageGetWidth(Image);
	OutH = (int32)CGImageGetHeight(Image);
	if (OutW <= 0 || OutH <= 0) { CGImageRelease(Image); return false; }

	// Render into a raw RGBA buffer via a CGBitmapContext
	TArray<uint8> RawRGBA;
	RawRGBA.SetNumZeroed(OutW * OutH * 4);

	CGColorSpaceRef ColorSpace = CGColorSpaceCreateDeviceRGB();
	CGContextRef Ctx = CGBitmapContextCreate(
		RawRGBA.GetData(),
		OutW, OutH,
		8,             // bits per component
		OutW * 4,      // bytes per row
		ColorSpace,
		kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big  // RGBX, big-endian → R,G,B,X in memory
	);
	CGColorSpaceRelease(ColorSpace);

	if (!Ctx) { CGImageRelease(Image); return false; }

	CGContextDrawImage(Ctx, CGRectMake(0, 0, OutW, OutH), Image);
	CGContextRelease(Ctx);
	CGImageRelease(Image);

	// Convert RGBX → FColor (B,G,R,A in memory)
	OutPixels.SetNumUninitialized(OutW * OutH);
	for (int32 i = 0; i < OutW * OutH; i++)
	{
		OutPixels[i].R = RawRGBA[i * 4 + 0];
		OutPixels[i].G = RawRGBA[i * 4 + 1];
		OutPixels[i].B = RawRGBA[i * 4 + 2];
		OutPixels[i].A = 255;
	}

	return true;
}

#endif // PLATFORM_MAC
