// Copyright CoSMoSoftware 2021. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class MillicastPlayerEditor : ModuleRules
	{
		public MillicastPlayerEditor(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

			if (ReadOnlyBuildVersion.Current.MajorVersion < 5)
			{
				CppStandard = CppStandardVersion.Cpp17;
			}
			
			PublicDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"MediaAssets",
					"UnrealEd"
				});

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"MillicastPlayer",
					"Engine",
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
