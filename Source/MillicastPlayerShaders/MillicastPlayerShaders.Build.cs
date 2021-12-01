// Copyright CoSMoSoftware 2021. All rights reserved

using System;
using System.IO;

using UnrealBuildTool;

public class MillicastPlayerShaders : ModuleRules
{
	public MillicastPlayerShaders(ReadOnlyTargetRules Target) : base(Target)
	{
		bEnforceIWYU = true;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Engine",
			"Core",
			"CoreUObject",
			"Projects",
			"InputCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Renderer",
			"RenderCore",
			"RHI"
		});
	}
}
