using UnrealBuildTool;

public class UECortex : ModuleRules
{
	public UECortex(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Always required — present in every UE editor build
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"HTTP",
			"HTTPServer",
			"Json",
			"JsonUtilities",
			"BlueprintGraph",
			"Kismet",
			"KismetCompiler",
			"AssetTools",
			"AssetRegistry",
			"Slate",
			"SlateCore",
			"EditorStyle",
			"LevelEditor",
			"SubobjectDataInterface",
			"NavigationSystem",
			"EnhancedInput",
			"HotReload",
		"ImageWrapper",
		"ApplicationCore",
		"OnlineSubsystem",
		});

		// Mac: CoreGraphics + AppKit for editor window capture and input simulation
		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicFrameworks.AddRange(new string[] { "CoreGraphics", "AppKit", "Foundation" });
		}

		// PCG and GAS are handled by UECortexPCG / UECortexGAS submodules — no dependency here.

		// Future optional modules — compile succeeds if absent
		DynamicallyLoadedModuleNames.AddRange(new string[]
		{
			"Niagara",
			"NiagaraEditor",
			"UMGEditor",
			"UMG",
			"MovieScene",
			"MovieSceneTracks",
			"LevelSequence",
			"Sequencer",
			"GeometryScriptingCore",
			"GeometryCore",
			"AnimGraph",
			"AnimationBlueprintLibrary",
			"PhysicsCore",
		});
	}
}
