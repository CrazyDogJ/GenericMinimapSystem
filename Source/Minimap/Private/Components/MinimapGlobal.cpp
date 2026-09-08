// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/MinimapGlobal.h"

#include "Net/UnrealNetwork.h"

UMinimapGlobal::UMinimapGlobal()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMinimapGlobal::UpdatePinStateList()
{
	PinStateList.UpdateAllTransforms();
}

void UMinimapGlobal::BeginPlay()
{
	PoiStateList.WorldContextObject = this;
	PinStateList.OwnerObject = this;
	
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(PinStateUpdateTimer, 
			FTimerDelegate::CreateUObject(this, &ThisClass::UpdatePinStateList), PinStateUpdatePeriod, true);
	}
}

void UMinimapGlobal::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PoiStateList)
	DOREPLIFETIME(ThisClass, PinStateList)
}
