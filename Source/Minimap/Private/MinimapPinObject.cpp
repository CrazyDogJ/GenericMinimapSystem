// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapPinObject.h"

#include "Components/MinimapGlobal.h"
#include "Net/UnrealNetwork.h"

void UMinimapPinObject::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, ReplicatingData);
}

UMinimapGlobal* UMinimapPinObject::GetOwningMinimapGlobal() const
{
	return Cast<UMinimapGlobal>(GetOwnerComponent());
}

void UMinimapPinObject::NativeBeginPlay()
{
	if (const auto Global = GetOwningMinimapGlobal())
	{
		Global->OnMapPinAddEvent.Broadcast(Id);
	}
	
	Super::NativeBeginPlay();
}

void UMinimapPinObject::NativeEndPlay()
{
	if (const auto Global = GetOwningMinimapGlobal())
	{
		Global->OnMapPinRemoveEvent.Broadcast(Id);
	}
	
	Super::NativeEndPlay();
}

void UMinimapPinObject::UpdateTransform()
{
	if (ReplicatingData.AttachedComponent.IsValid())
	{
		ReplicatingData.Location = ReplicatingData.AttachedComponent->GetComponentLocation();
		ReplicatingData.Yaw = ReplicatingData.AttachedComponent->GetComponentRotation().Yaw;
	}
}

void UMinimapPinObject::OnRep_ReplicatingData()
{
	OnReplicatingDataChanged();
}
