using UnrealBuildTool;

public class UECortexPCG : ModuleRules
{
	public UECortexPCG(ReadOnlyTargetRules Target) : base(Target)
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
			"UECortex",   // FMCPToolRegistry + FMCPToolBase
			"PCG",
			"PCGEditor",
		});
	}
}
