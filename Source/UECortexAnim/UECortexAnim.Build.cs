using UnrealBuildTool;

public class UECortexAnim : ModuleRules
{
	public UECortexAnim(ReadOnlyTargetRules Target) : base(Target)
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
			"UECortex",
			"AnimGraph",
			"AnimGraphRuntime",
			"Kismet",
			"BlueprintGraph",
		});
	}
}
