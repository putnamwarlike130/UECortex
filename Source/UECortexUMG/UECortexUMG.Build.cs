using UnrealBuildTool;

public class UECortexUMG : ModuleRules
{
	public UECortexUMG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Json",
			"JsonUtilities",
			"AssetRegistry",
			"AssetTools",
			"UECortex",      // FMCPToolRegistry + FMCPToolBase
			"UMG",
			"UMGEditor",
			"Slate",
			"SlateCore",
			"Kismet",
			"BlueprintGraph",
		});
	}
}
