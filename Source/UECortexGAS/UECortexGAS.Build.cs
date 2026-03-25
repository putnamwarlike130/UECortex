using UnrealBuildTool;

public class UECortexGAS : ModuleRules
{
	public UECortexGAS(ReadOnlyTargetRules Target) : base(Target)
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
			"UECortex",            // FMCPToolRegistry + FMCPToolBase
			"GameplayAbilities",
			"GameplayTasks",
			"GameplayTags",
		});
	}
}
