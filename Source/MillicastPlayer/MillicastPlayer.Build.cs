// Copyright CoSMoSoftware 2021. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
    using System;
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

			bool bUseMillicastWebRTC = true;

			try
			{
				String modulePath = GetModuleDirectory("MillicastWebRTC"); // This throw an exception if the module does not exists
				bUseMillicastWebRTC = !modulePath.Equals("");
            }
            catch(Exception)
			{
				bUseMillicastWebRTC = false;
			}

            if (bUseMillicastWebRTC)
			{
				Console.WriteLine("The plugin will link against MillicastWebRTC");
                PublicDependencyModuleNames.AddRange(new string[] { "MillicastWebRTC" });
                PublicDefinitions.Add("WITH_MILLICAST_WEBRTC=1");
            }
			else
			{
                Console.WriteLine("The plugin will link against UnrealEngine WebRTC module");
                PublicDependencyModuleNames.AddRange(new string[] { "WebRTC" });
				PublicDefinitions.Add("WEBRTC_VERSION=96");
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
					"HeadMountedDisplay",
					"AVCodecsCore",
					"AVCodecsCoreRHI"
		});

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"Media",
				});

			if(!bUseMillicastWebRTC && Target.Platform.ToString() != "Mac")
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
