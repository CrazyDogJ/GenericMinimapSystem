// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/MinimapGlobal.h"

#include "Net/UnrealNetwork.h"

UMinimapGlobal::UMinimapGlobal()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMinimapGlobal::BeginPlay()
{
	PoiStateList.WorldContextObject = this;
	
	Super::BeginPlay();
}

void UMinimapGlobal::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PoiStateList)
}
