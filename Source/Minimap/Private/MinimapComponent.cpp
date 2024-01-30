// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapComponent.h"
#include "MinimapSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UMinimapComponent::UMinimapComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}


void UMinimapComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMinimapComponent, PinSlateBrush);
	DOREPLIFETIME(UMinimapComponent, bRotate);
	DOREPLIFETIME(UMinimapComponent, bAlwaysShow);
	DOREPLIFETIME(UMinimapComponent, TempPin);
	DOREPLIFETIME(UMinimapComponent, UniqueColorIndex);
}

UMinimapSubsystem* UMinimapComponent::GetMinimapSubsystem() const
{
	if (GetWorld() && GetWorld()->GetGameInstance())
	{
		return GetWorld()->GetGameInstance()->GetSubsystem<UMinimapSubsystem>();
	}

	return nullptr;
}

APlayerState* UMinimapComponent::GetPlayerState() const
{
	if (Cast<APawn>(GetOwner()))
	{
		return Cast<APawn>(GetOwner())->GetPlayerState();
	}
	return nullptr;
}

bool UMinimapComponent::IsLocalControlled() const
{
	return Cast<APawn>(GetOwner())->IsLocallyControlled();
}

// Called when the game starts
void UMinimapComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (UMinimapSubsystem* MinimapSubsystem = GetMinimapSubsystem())
	{
		MinimapSubsystem->OnComponentRegistered.AddDynamic(this, &UMinimapComponent::OnCompReg);
		MinimapSubsystem->OnComponentUnregistered.AddDynamic(this, &UMinimapComponent::OnCompUnreg);
		MinimapSubsystem->RegisterComponent(this);
	}
}


void UMinimapComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMinimapSubsystem* MinimapSubsystem = GetMinimapSubsystem())
	{
		MinimapSubsystem->UnregisterComponent(this);
	}
	Super::EndPlay(EndPlayReason);
}
