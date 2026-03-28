#include "Tools/MCPViewportCaptureTools.h"
#include "Editor.h"
#include "LevelEditorViewport.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/Base64.h"
#include "Modules/ModuleManager.h"
#include "Application/SlateApplicationBase.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

void FMCPViewportCaptureTools::RegisterTools(TArray<FMCPToolDef>& OutTools)
{
	{
		FMCPToolDef T;
		T.Name = TEXT("viewport_capture");
		T.Description = TEXT(
			"Capture the Unreal Editor viewport or full editor window as an image. "
			"mode='viewport' (default) captures the 3D render only. "
			"mode='editor' captures the entire editor window including Fab browser, Content Browser, panels etc. "
			"width/height control output resolution (default 1280x720). Set height=0 to auto-calculate from aspect ratio."
		);
		T.Params = {
			{ TEXT("mode"),   TEXT("string"),  TEXT("'viewport' (3D render) or 'editor' (full window). Default: viewport"), false },
			{ TEXT("width"),  TEXT("number"),  TEXT("Output width in pixels. Default: 1280"), false },
			{ TEXT("height"), TEXT("number"),  TEXT("Output height in pixels. 0 = auto from aspect ratio. Default: 720"), false },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return ViewportCapture(P); };
		OutTools.Add(T);
	}
}

FMCPToolResult FMCPViewportCaptureTools::ViewportCapture(const TSharedPtr<FJsonObject>& Params)
{
	// Parse params
	FString Mode = TEXT("viewport");
	Params->TryGetStringField(TEXT("mode"), Mode);

	int32 TargetW = 1280;
	int32 TargetH = 720;
	if (Params->HasField(TEXT("width")))  TargetW = (int32)Params->GetNumberField(TEXT("width"));
	if (Params->HasField(TEXT("height"))) TargetH = (int32)Params->GetNumberField(TEXT("height"));

	TargetW = FMath::Clamp(TargetW, 64, 3840);

	// Capture native pixels
	TArray<FColor> NativePixels;
	int32 NativeW = 0, NativeH = 0;
	bool bOK = false;

	if (Mode == TEXT("editor"))
	{
		bOK = CaptureEditorWindow(NativePixels, NativeW, NativeH);
	}
	else
	{
		bOK = CaptureViewport(NativePixels, NativeW, NativeH);
	}

	if (!bOK || NativePixels.Num() == 0)
	{
		return FMCPToolResult::Error(TEXT("Capture failed — no active viewport or editor window found"));
	}

	// Auto-calculate height if requested
	if (TargetH <= 0 && NativeH > 0)
	{
		TargetH = FMath::RoundToInt((float)TargetW * (float)NativeH / (float)NativeW);
	}
	TargetH = FMath::Clamp(TargetH, 64, 2160);

	// Resize
	TArray<FColor> OutPixels;
	if (TargetW != NativeW || TargetH != NativeH)
	{
		ResizePixels(NativePixels, NativeW, NativeH, OutPixels, TargetW, TargetH);
	}
	else
	{
		OutPixels = MoveTemp(NativePixels);
	}

	// Encode to base64 PNG
	FString Base64;
	if (!EncodePNG(OutPixels, TargetW, TargetH, Base64))
	{
		return FMCPToolResult::Error(TEXT("PNG encoding failed"));
	}

	return FMCPToolResult::Image(Base64);
}

bool FMCPViewportCaptureTools::CaptureViewport(TArray<FColor>& OutPixels, int32& OutW, int32& OutH)
{
	if (!GEditor) return false;

	FViewport* Viewport = GEditor->GetActiveViewport();
	if (!Viewport) return false;

	OutW = Viewport->GetSizeXY().X;
	OutH = Viewport->GetSizeXY().Y;
	if (OutW <= 0 || OutH <= 0) return false;

	return Viewport->ReadPixels(OutPixels, FReadSurfaceDataFlags());
}

// CaptureEditorWindow — Windows impl here. Mac impl is in MCPViewportCaptureTools.mm
#if PLATFORM_WINDOWS

bool FMCPViewportCaptureTools::CaptureEditorWindow(TArray<FColor>& OutPixels, int32& OutW, int32& OutH)
{
	// Find the top-level UE editor window
	TSharedPtr<SWindow> TopWindow;
	if (FSlateApplication::IsInitialized())
	{
		TArray<TSharedRef<SWindow>> Windows;
		FSlateApplication::Get().GetAllVisibleWindowsOrdered(Windows);
		for (const TSharedRef<SWindow>& Win : Windows)
		{
			if (Win->GetType() == EWindowType::Normal)
			{
				TopWindow = Win;
				break;
			}
		}
	}

	if (!TopWindow.IsValid()) return false;

	HWND Hwnd = (HWND)TopWindow->GetNativeWindow()->GetOSWindowHandle();
	if (!Hwnd) return false;

	// Get window rect on screen
	RECT WinRect;
	GetWindowRect(Hwnd, &WinRect);
	OutW = WinRect.right  - WinRect.left;
	OutH = WinRect.bottom - WinRect.top;
	if (OutW <= 0 || OutH <= 0) return false;

	// Capture from screen DC (works with DWM-composited DX content)
	HDC hdcScreen = GetDC(NULL);
	HDC hdcMem    = CreateCompatibleDC(hdcScreen);
	HBITMAP hBmp  = CreateCompatibleBitmap(hdcScreen, OutW, OutH);
	HGDIOBJ hOld  = SelectObject(hdcMem, hBmp);

	BitBlt(hdcMem, 0, 0, OutW, OutH, hdcScreen, WinRect.left, WinRect.top, SRCCOPY);

	// Read pixels (BGRA)
	BITMAPINFOHEADER bi = {};
	bi.biSize        = sizeof(BITMAPINFOHEADER);
	bi.biWidth       = OutW;
	bi.biHeight      = -OutH;   // top-down
	bi.biPlanes      = 1;
	bi.biBitCount    = 32;
	bi.biCompression = BI_RGB;

	TArray<uint8> RawBGRA;
	RawBGRA.SetNumUninitialized(OutW * OutH * 4);
	GetDIBits(hdcMem, hBmp, 0, OutH, RawBGRA.GetData(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	SelectObject(hdcMem, hOld);
	DeleteObject(hBmp);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);

	// Convert BGRA → FColor (RGBA)
	OutPixels.SetNumUninitialized(OutW * OutH);
	for (int32 i = 0; i < OutW * OutH; i++)
	{
		OutPixels[i].B = RawBGRA[i * 4 + 0];
		OutPixels[i].G = RawBGRA[i * 4 + 1];
		OutPixels[i].R = RawBGRA[i * 4 + 2];
		OutPixels[i].A = 255;
	}

	return true;
}

#elif !PLATFORM_MAC

// Linux / other — not supported (Mac handled in .mm)
bool FMCPViewportCaptureTools::CaptureEditorWindow(TArray<FColor>& OutPixels, int32& OutW, int32& OutH)
{
	return false;
}

#endif // PLATFORM_WINDOWS / !PLATFORM_MAC

void FMCPViewportCaptureTools::ResizePixels(const TArray<FColor>& Src, int32 SrcW, int32 SrcH,
                                             TArray<FColor>& Dst, int32 DstW, int32 DstH)
{
	FImageUtils::ImageResize(SrcW, SrcH, Src, DstW, DstH, Dst, false);
}

bool FMCPViewportCaptureTools::EncodePNG(const TArray<FColor>& Pixels, int32 Width, int32 Height, FString& OutBase64)
{
	IImageWrapperModule* IWM = FModuleManager::GetModulePtr<IImageWrapperModule>(TEXT("ImageWrapper"));
	if (!IWM) return false;

	TSharedPtr<IImageWrapper> Wrapper = IWM->CreateImageWrapper(EImageFormat::PNG);
	if (!Wrapper.IsValid()) return false;

	if (!Wrapper->SetRaw(Pixels.GetData(), Pixels.Num() * sizeof(FColor),
	                     Width, Height, ERGBFormat::BGRA, 8))
	{
		return false;
	}

	TArray64<uint8> Compressed = Wrapper->GetCompressed();
	if (Compressed.Num() == 0) return false;

	OutBase64 = FBase64::Encode(Compressed.GetData(), Compressed.Num());
	return true;
}
