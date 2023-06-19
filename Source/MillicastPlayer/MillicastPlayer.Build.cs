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
			
			if (ReadOnlyBuildVersion.Current.MajorVersion < 5)
			{
				CppStandard = CppStandardVersion.Cpp17;
			}

			DynamicallyLoadedModuleNames.AddRange(
				new string[] {
					"Media",
				});

			PublicDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"MediaAssets",
					"OpenSSL",
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
					"libOpus",
					"HeadMountedDisplay"
		});

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"Media",
				});

			if( Target.Platform.ToString() != "Mac")
			{
				PrivateIncludePaths.AddRange(
					new string[] {
					    "MillicastPlayer/Private",
                    			    Path.Combine(Path.GetFullPath(Target.RelativeEnginePath), "Source/ThirdParty/WebRTC/4664/Include/third_party/libyuv/include"), // for libyuv headers
				});
			}
		}
	}
}
