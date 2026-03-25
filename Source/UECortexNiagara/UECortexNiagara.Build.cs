using UnrealBuildTool;

public class UECortexNiagara : ModuleRules
{
	public UECortexNiagara(ReadOnlyTargetRules Target) : base(Target)
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
			"UECortex",      // FMCPToolRegistry + FMCPToolBase
			"Niagara",
			"NiagaraEditor",
		});
	}
}
