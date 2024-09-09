// Infinity Reply, All Rights Reserved

using UnrealBuildTool;

public class InfinityAI : ModuleRules
{
    public InfinityAI(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(new string[] { "WebSockets", "JsonUtilities", "Json", "glTFRuntime", "glTFRuntimeOBJ" });
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "HTTP" });

        PublicIncludePaths.AddRange(new string[] { "InfinityAI" });
    }
}
