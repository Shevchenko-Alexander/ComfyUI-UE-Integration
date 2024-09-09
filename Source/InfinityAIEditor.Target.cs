// Infinity Reply, All Rights Reserved

using UnrealBuildTool;
using System.Collections.Generic;

public class InfinityAIEditorTarget : TargetRules
{
    public InfinityAIEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V4;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new string[] { "InfinityAI" });
    }
}
