// Copyright CoSMoSoftware 2021. All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "RHI.h"
#include "RenderResource.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"

class FMillicastShaderVS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FMillicastShaderVS, Global, MILLICASTPLAYERSHADERS_API);

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	FMillicastShaderVS()
	{}

	FMillicastShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{}
};


class FMillicastShaderPS : public FGlobalShader
{
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	FMillicastShaderPS()
	{}

	FMillicastShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{}

	struct Params
	{
		Params(const TRefCountPtr<FRHITexture2D>& InputTargetIn, FIntPoint OutputSizeIn, bool LinearToSrgbIn)
			: InputTarget(InputTargetIn)
			, OutputSize(OutputSizeIn)
			, LinearToSrgb(LinearToSrgbIn)
		{}

		TRefCountPtr<FRHITexture2D> InputTarget;
		FIntPoint OutputSize;
		bool LinearToSrgb;
	};

	MILLICASTPLAYERSHADERS_API void SetParameters(FRHICommandList& CommandList, const Params& params);

protected:
};


class FMillicastShaderBGRAtoUYVYPS : public FMillicastShaderPS
{
	DECLARE_EXPORTED_SHADER_TYPE(FMillicastShaderBGRAtoUYVYPS, Global, MILLICASTPLAYERSHADERS_API);

public:
	using FMillicastShaderPS::FMillicastShaderPS;
};

class FMillicastShaderUYVYtoBGRAPS : public FMillicastShaderPS
{
	DECLARE_EXPORTED_SHADER_TYPE(FMillicastShaderUYVYtoBGRAPS, Global, MILLICASTPLAYERSHADERS_API);

public:
	using FMillicastShaderPS::FMillicastShaderPS;
};


class IMillicastShaders : public IModuleInterface
{
public:
};
