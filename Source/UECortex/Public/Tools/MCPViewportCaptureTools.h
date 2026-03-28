#pragma once
#include "MCPToolBase.h"

class UECORTEX_API FMCPViewportCaptureTools : public FMCPToolModuleBase
{
public:
	virtual FString GetModuleName() const override { return TEXT("viewport_capture"); }
	virtual void RegisterTools(TArray<FMCPToolDef>& OutTools) override;

private:
	static FMCPToolResult ViewportCapture(const TSharedPtr<FJsonObject>& Params);

	// Encode a pixel array to base64 PNG at the given dimensions
	static bool EncodePNG(const TArray<FColor>& Pixels, int32 Width, int32 Height, FString& OutBase64);

	// Resize pixels from (SrcW x SrcH) to (DstW x DstH)
	static void ResizePixels(const TArray<FColor>& Src, int32 SrcW, int32 SrcH,
	                         TArray<FColor>& Dst, int32 DstW, int32 DstH);

	// Capture the 3D editor viewport
	static bool CaptureViewport(TArray<FColor>& OutPixels, int32& OutW, int32& OutH);

	// Capture the full editor window via Windows GDI
	static bool CaptureEditorWindow(TArray<FColor>& OutPixels, int32& OutW, int32& OutH);
};
