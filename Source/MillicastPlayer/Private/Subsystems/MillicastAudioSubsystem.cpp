// Copyright Millicast 2023. All Rights Reserved.

#include "Subsystems/MillicastAudioSubsystem.h"
#include "Audio/MillicastAudioInstance.h"

namespace
{
	auto GetPredicate(UAudioComponent* Component)
	{
		const auto Predicate = [=](const auto* Entry) { return Entry->GetAudioComponent() == Component; };
		return Predicate;
	}
}

void UMillicastAudioSubsystem::Register(UAudioComponent* Component)
{
	if(AudioInstances.ContainsByPredicate(GetPredicate(Component)))
	{
		//UE_LOG(LogTemp, Error, TEXT("[UMillicastAudioSubsystem::Register] Tried to register a component that has already been registered"));
		return;
	}
	
	auto* Instance = NewObject<UMillicastAudioInstance>(this);
	Instance->InjectDependencies(Component);
	AudioInstances.Add(Instance);
}

void UMillicastAudioSubsystem::Unregister(UAudioComponent* Component)
{
	auto* It = AudioInstances.FindByPredicate(GetPredicate(Component));
	if(!It)
	{
		UE_LOG(LogTemp, Error, TEXT("[UMillicastAudioSubsystem::Unregister] Tried to unregister a component that has not been registered"));
		return;
	}

#if ENGINE_MAJOR_VERSION >= 5
	(*It)->MarkAsGarbage();
#else
	(*It)->MarkPendingKill();
#endif

	AudioInstances.RemoveSingleSwap(*It);
}

UMillicastAudioInstance* UMillicastAudioSubsystem::GetInstance(UAudioComponent* Component)
{
	auto* It = AudioInstances.FindByPredicate(GetPredicate(Component));
	if(!It)
	{
		UE_LOG(LogTemp, Error, TEXT("[UMillicastAudioSubsystem::GetInstance] Failed to find valid registered instance"));
		return nullptr;
	}
	
	return *It;
}
