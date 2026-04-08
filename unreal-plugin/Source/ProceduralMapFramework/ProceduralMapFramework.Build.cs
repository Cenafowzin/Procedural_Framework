using UnrealBuildTool;

public class ProceduralMapFramework : ModuleRules
{
    public ProceduralMapFramework(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Json",
            "JsonUtilities",
            "Projects",  // IPluginManager — para resolver o Content dir do plugin
        });
    }
}
