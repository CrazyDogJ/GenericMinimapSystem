// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/MinimapComponent.h"

#include "MinimapSubsystem.h"
#include "Components/MinimapGlobal.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UMinimapComponent::UMinimapComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAllowAnyoneToDestroyMe = true;
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}


void UMinimapComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, MinimapGuid)
}

UMinimapSubsystem* UMinimapComponent::GetMinimapSubsystem() const
{
	return GetWorld()->GetSubsystem<UMinimapSubsystem>();
}

bool UMinimapComponent::ShouldVisible_Implementation()
{
	return true;
}

void UMinimapComponent::GetDisplayNameAndDescription_Implementation(FText& DisplayName, FText& Description) const
{
	NativeGetDisplayNameAndDescription(DisplayName, Description);
}

FMapPinStateEntry UMinimapComponent::MakeMapPinEntry() const
{
	FMapPinStateEntry NewEntry;
	NewEntry.Id = MinimapGuid;
	if (const AActor* Owner = GetOwner())
	{
		NewEntry.ActorWeakPtr = Owner;
		UE_LOG(LogTemp, Warning, TEXT("The actor weak ptr is : %s"), *NewEntry.ActorWeakPtr->GetName())
	}
	NewEntry.Brush = PinSlateBrush;
	NewEntry.bHasYaw = bRotate;
	NewEntry.bAddToOverlay = bAddToOverlay;
	NewEntry.bAlwaysOnMinimap = bAlwaysShow;
	NewEntry.CategoryTag = MinimapCategory;
	GetDisplayNameAndDescription(NewEntry.PinName, NewEntry.PinDescription);
	return NewEntry;
}

void UMinimapComponent::RegisterGlobalMinimap() const
{
	if (GetIsReplicated())
	{
		const auto MG = GetGlobalMinimapComponent();
		if (MG && GetOwner()->HasAuthority())
		{
			MG->PinStateList.AddMapPinState(MakeMapPinEntry());
		}
	}
	else
	{
		if (const auto MS = GetMinimapSubsystem())
		{
			MS->LocalPinStateList.AddMapPinState(MakeMapPinEntry());
		}
	}
}

void UMinimapComponent::UnregisterGlobalMinimap() const
{
	if (GetIsReplicated())
	{
		const auto MG = GetGlobalMinimapComponent();
		if (MG && GetOwner()->HasAuthority())
		{
			MG->PinStateList.RemoveMapPinState(MinimapGuid);
		}
	}
	else
	{
		if (const auto MS = GetMinimapSubsystem())
		{
			MS->LocalPinStateList.RemoveMapPinState(MinimapGuid);
		}
	}
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

UMinimapGlobal* UMinimapComponent::GetGlobalMinimapComponent() const
{
	if (const auto World = GetWorld())
	{
		if (const auto GS = World->GetGameState())
		{
			return GS->GetComponentByClass<UMinimapGlobal>();
		}
	}
	
	return nullptr;
}

// Called when the game starts
void UMinimapComponent::BeginPlay()
{
	Super::BeginPlay();

	// Init guid when begin play(if not valid)
	if (!MinimapGuid.IsValid())
	{
		MinimapGuid = FGuid::NewGuid();
	}

	RegisterGlobalMinimap();
}

void UMinimapComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterGlobalMinimap();
	
	Super::EndPlay(EndPlayReason);
}
