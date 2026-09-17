// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/MinimapReceiver.h"

#include "MinimapSubsystem.h"
#include "Net/UnrealNetwork.h"

UMinimapReceiver::UMinimapReceiver()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicated(true);
}

void UMinimapReceiver::AddChannel(uint8 InChannel)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	
	if (Channels.Contains(InChannel))
	{
		return;
	}

	Channels.Add(InChannel);
	if (const auto Sub = GetMinimapSubsystem())
	{
		for (const auto Itr : Sub->GetReplicatingMapPinStates())
		{
			if (ShouldReplicate(Itr))
			{
				ReplicatedMapPinList.AddMapPinState(Itr);
			}
		}
	}
}

void UMinimapReceiver::RemoveChannel(uint8 InChannel)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	
	if (!Channels.Contains(InChannel))
	{
		return;
	}
	
	Channels.Remove(InChannel);
	for (const auto Itr : ReplicatedMapPinList.StateEntries)
	{
		if (!ShouldReplicate(Itr))
		{
			ReplicatedMapPinList.RemoveMapPinState(Itr.Id);
		}
	}
}

UMinimapSubsystem* UMinimapReceiver::GetMinimapSubsystem() const
{
	return GetWorld()->GetSubsystem<UMinimapSubsystem>();
}

bool UMinimapReceiver::ShouldReplicate(const FMapPinStateEntry& InEntry) const
{
	return Channels.Contains(InEntry.Channel);
}

void UMinimapReceiver::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		if (const auto Sub = GetMinimapSubsystem())
		{
			for (const auto Itr : Sub->GetReplicatingMapPinStates())
			{
				ReplicatedMapPinList.AddMapPinState(Itr);
			}
			
			Sub->RegisterReceiver(this);
		}
	}
}

void UMinimapReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner()->HasAuthority())
	{
		if (const auto Sub = GetMinimapSubsystem())
		{
			Sub->UnregisterReceiver(this);
		}
	}
	
	Super::EndPlay(EndPlayReason);
}

void UMinimapReceiver::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.Condition = COND_OwnerOnly;
	Params.RepNotifyCondition = REPNOTIFY_Always;
	
	DOREPLIFETIME_WITH_PARAMS(ThisClass, ReplicatedMapPinList, Params);
	DOREPLIFETIME_WITH_PARAMS(ThisClass, Channels, Params);
}
