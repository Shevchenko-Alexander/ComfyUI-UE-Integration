// Infinity Reply, All Rights Reserved

using UnrealBuildTool;
using System.Collections.Generic;

public class InfinityAITarget : TargetRules
{
    public InfinityAITarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V4;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new string[] { "InfinityAI" });
    }
}
