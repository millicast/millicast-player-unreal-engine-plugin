// Copyright CoSMoSoftware 2021. All rights reserved

#include "MillicastShaders.h"

#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#include "Misc/Paths.h"
#include "Misc/EngineVersionComparison.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "PipelineStateCache.h"
#include "SceneUtils.h"
#include "SceneInterface.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FMillicastShaderUB, )
	SHADER_PARAMETER(uint32, InputWidth)
	SHADER_PARAMETER(uint32, InputHeight)
	SHADER_PARAMETER(uint32, OutputWidth)
	SHADER_PARAMETER(uint32, OutputHeight)
	SHADER_PARAMETER(uint32, LinearToSrgb)
	SHADER_PARAMETER_TEXTURE(Texture2D, InputTarget)
	SHADER_PARAMETER_SAMPLER(SamplerState, SamplerP)
	SHADER_PARAMETER_SAMPLER(SamplerState, SamplerB)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FMillicastShaderUB, "MillicastShaderUB");

IMPLEMENT_GLOBAL_SHADER(FMillicastShaderVS, "/Plugin/MillicastPlayer/Private/MillicastShaders.usf", "MillicastMainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FMillicastShaderBGRAtoUYVYPS, "/Plugin/MillicastPlayer/Private/MillicastShaders.usf", "MillicastBGRAtoUYVYPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FMillicastShaderUYVYtoBGRAPS, "/Plugin/MillicastPlayer/Private/MillicastShaders.usf", "MillicastUYVYtoBGRAPS", SF_Pixel);



void FMillicastShaderPS::SetParameters(FRHICommandList& CommandList, const Params& params)
{
	FMillicastShaderUB UB;
	{
		UB.InputWidth = params.InputTarget->GetSizeX();
		UB.InputHeight = params.InputTarget->GetSizeY();
		UB.OutputWidth = params.OutputSize.X;
		UB.OutputHeight = params.OutputSize.Y;
		UB.LinearToSrgb = params.LinearToSrgb;
		UB.InputTarget = params.InputTarget;
		UB.SamplerP = TStaticSamplerState<SF_Trilinear>::GetRHI();
		UB.SamplerB = TStaticSamplerState<SF_Bilinear>::GetRHI();
	}

	TUniformBufferRef<FMillicastShaderUB> Data = TUniformBufferRef<FMillicastShaderUB>::CreateUniformBufferImmediate(UB, UniformBuffer_SingleFrame);
	SetUniformBufferParameter(CommandList, CommandList.GetBoundPixelShader(), GetUniformBufferParameter<FMillicastShaderUB>(), Data);
}


class FMillicastShaders : public IMillicastShaders
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{
		FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("MillicastPlayer"))->GetBaseDir(), TEXT("Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/Plugin/MillicastPlayer"), PluginShaderDir);
   }
        virtual void ShutdownModule() override
        {
        }
};

IMPLEMENT_MODULE( FMillicastShaders, MillicastShaders )
