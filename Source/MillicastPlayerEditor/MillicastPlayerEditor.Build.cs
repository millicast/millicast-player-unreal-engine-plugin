// Copyright CoSMoSoftware 2021. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class MillicastPlayerEditor : ModuleRules
	{
		public MillicastPlayerEditor(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"UnrealEd"
				});

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"MillicastPlayer",
					"Engine",
					"MediaAssets",
					"Projects",
					"SlateCore",
					"PlacementMode",
				});

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"AssetTools",
				});

			PrivateIncludePaths.Add("MillicastPlayerEditor/Private");
		}
	}
}
