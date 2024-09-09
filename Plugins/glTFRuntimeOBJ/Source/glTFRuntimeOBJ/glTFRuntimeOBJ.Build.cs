// Copyright 2023, Roberto De Ioris.

using UnrealBuildTool;

public class glTFRuntimeOBJ : ModuleRules
{
	public glTFRuntimeOBJ(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"glTFRuntime"
				// ... add private dependencies that you statically link with here ...	
			}
			);


        if (Target.Version.MajorVersion >= 5)
        {
            PrivateDependencyModuleNames.Add("GeometryCore");
        }
		else
		{
            PrivateDependencyModuleNames.Add("GeometricObjects");
        }


        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
