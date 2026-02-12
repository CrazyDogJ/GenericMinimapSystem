// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/MinimapComponent.h"

#include "MinimapBlueprintFunctionLibrary.h"
#include "MinimapSubsystem.h"
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

	DOREPLIFETIME(UMinimapComponent, PinSlateBrush);
	DOREPLIFETIME(UMinimapComponent, bRotate);
	DOREPLIFETIME(UMinimapComponent, bAlwaysShow);
	DOREPLIFETIME(UMinimapComponent, UniqueColorIndex);
	DOREPLIFETIME(UMinimapComponent, bAddToOverlay);
}

FStaticMapPin UMinimapComponent::GetCurrentStaticMapPin()
{
	FStaticMapPin Result;
	Result.IdentifyGuid = MinimapGuid;
	Result.Location = GetOwner()->GetActorLocation();
	Result.Yaw = GetOwner()->GetActorRotation().Yaw;
	Result.bHasRotation = bRotate;
	Result.CategoryTag = MinimapCategory;
	Result.bAddToOverlay = bAddToOverlay;
	Result.bAlwaysOnMinimap = bAlwaysShow;
	Result.MapPinBrush = PinSlateBrush;
	//TODO:Name and description;
	return Result;
}

bool UMinimapComponent::ShouldVisible_Implementation()
{
	return true;
}

void UMinimapComponent::GetDisplayNameAndDescription_Implementation(FText& DisplayName, FText& Description)
{
	NativeGetDisplayNameAndDescription(DisplayName, Description);
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

	// Init guid when begin play(if not valid)
	if (!MinimapGuid.IsValid())
	{
		MinimapGuid = FGuid::NewGuid();
	}

	TArray<UMinimapSubsystem*> Result;
	for (auto PlayerIt = GetWorld()->GetGameInstance()->GetLocalPlayerIterator(); PlayerIt; ++PlayerIt)
	{
		const ULocalPlayer* Player = *PlayerIt;
		const auto MinimapSubsystem = Player->GetSubsystem<UMinimapSubsystem>();
		Result.Add(MinimapSubsystem);
	}

	const auto GI = GetWorld()->GetGameInstance();
	UMinimapBlueprintFunctionLibrary::ForEachLocalPlayerSubsystem
	<UMinimapSubsystem>(GI, [this](UMinimapSubsystem* Subsystem)
	{
		Subsystem->RegisterComponent(this);
	});
}

void UMinimapComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	const auto GI = GetWorld()->GetGameInstance();
	UMinimapBlueprintFunctionLibrary::ForEachLocalPlayerSubsystem
	<UMinimapSubsystem>(GI, [this](UMinimapSubsystem* Subsystem)
	{
		Subsystem->UnregisterComponent(this);
	});
	
	Super::EndPlay(EndPlayReason);
}
