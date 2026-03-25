using UnrealBuildTool;

public class UECortexSequencer : ModuleRules
{
	public UECortexSequencer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "UnrealEd",
			"Json", "JsonUtilities", "AssetRegistry",
			"UECortex",
			"LevelSequence",
			"MovieScene",
			"MovieSceneTracks",
			"CinematicCamera",
		});
	}
}
