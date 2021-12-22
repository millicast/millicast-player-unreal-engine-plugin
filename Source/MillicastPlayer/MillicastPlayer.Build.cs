// Copyright CoSMoSoftware 2021. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
	using System.IO;

	public class MillicastPlayer : ModuleRules
	{
		public MillicastPlayer(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
			PrivatePCHHeaderFile = "Private/PCH.h";


			DynamicallyLoadedModuleNames.AddRange(
				new string[] {
					"Media",
				});

			PublicDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"MediaAssets",
					"TimeManagement",
					"WebRTC",
					"RenderCore"
				});

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Engine",
					"MediaUtils",
					"MediaIOCore",
					"Projects",
					"SlateCore",
					"AudioMixer",
					"WebSockets",
					"HTTP",
					"Json",
					"SSL",
					"RHI",
					"HeadMountedDisplay"
		});

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"Media",
				});

			PrivateIncludePaths.AddRange(
				new string[] {
					"MillicastPlayer/Private",
				});
		}
	}
}
