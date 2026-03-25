using UnrealBuildTool;

public class UECortexPython : ModuleRules
{
	public UECortexPython(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "UnrealEd",
			"Json", "JsonUtilities",
			"UECortex",
			"PythonScriptPlugin",
		});
	}
}
