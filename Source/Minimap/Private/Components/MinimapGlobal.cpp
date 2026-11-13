// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/MinimapGlobal.h"

#include "Minimap.h"
#include "Components/MinimapPlayerComponent.h"
#include "GameFramework/PlayerController.h"
#include "MinimapPinObject.h"
#include "MinimapSubsystem.h"
#include "NetRelevantObject.h"
#include "NetRelevantObjectUtils.h"
#include "Actors/MapPinActor.h"
#include "Net/UnrealNetwork.h"

UMinimapGlobal::UMinimapGlobal()
	: Super()
{
	PrimaryComponentTick.bCanEverTick = true;
}

FGuid UMinimapGlobal::AddReplicatedMapPin(const TSubclassOf<UMinimapPinObject> PinClass, APlayerController* OwnerController,
	const FName GroupName, const FMinimapPinData& PinData)
{
	if (PinClass)
	{
		auto NewPin = AddNetRelevantObject(PinClass, OwnerController, GroupName);
		if (const auto MapPinObject = Cast<UMinimapPinObject>(NewPin))
		{
			MapPinObject->ReplicatingData = PinData;
			BeginObjectLogic(NewPin);
			return MapPinObject->Id;
		}
	}
	
	return FGuid();
}

FGuid UMinimapGlobal::AddLocalMapPin(const FMinimapPinData& PinData)
{
	const auto NewGuid = FGuid::NewGuid();
	LocalMapPins.Add(NewGuid, PinData);
	OnMapPinAddEvent.Broadcast(NewGuid);
	return NewGuid;
}

void UMinimapGlobal::UpdateLocalMapPins()
{
	for (auto& LocalMapPin : LocalMapPins)
	{
		if (LocalMapPin.Value.AttachedComponent.IsValid())
		{
			LocalMapPin.Value.Location = LocalMapPin.Value.AttachedComponent->GetComponentLocation();
			LocalMapPin.Value.Yaw = LocalMapPin.Value.AttachedComponent->GetComponentRotation().Yaw;
		}
	}
}

void UMinimapGlobal::AddLocalMapPinWithId(const FGuid Id, const FMinimapPinData& PinData)
{
	if (Id.IsValid())
	{
		LocalMapPins.Add(Id, PinData);
		OnMapPinAddEvent.Broadcast(Id);
	}
}

void UMinimapGlobal::RemoveLocalMapPin(const FGuid Id)
{
	if (Id.IsValid())
	{
		OnMapPinRemoveEvent.Broadcast(Id);
		LocalMapPins.Remove(Id);
	}
}

namespace
{
UMinimapPlayerComponent* GetRequestingPlayer(const UMinimapGlobal* Global, AController* Controller)
{
	auto* PC = Cast<APlayerController>(Controller);
	if (!IsValid(PC) || PC->GetWorld() != Global->GetWorld() || !PC->IsLocalPlayerController()) return nullptr;
	return PC->GetComponentByClass<UMinimapPlayerComponent>();
}
}

void UMinimapGlobal::ChangeTempPinLocation(AController* Controller, const FVector& Location)
{
	if (!IsValid(Controller) || Controller->GetWorld() != GetWorld() || Location.ContainsNaN()) return;
	if (!GetOwner()->HasAuthority())
	{
		if (auto* Player = GetRequestingPlayer(this, Controller)) Player->ServerMoveTempPin(Location);
		return;
	}
	if (auto* Pin = TempMapPinMapping.FindRef(Controller); IsValid(Pin)) Pin->SetActorLocation(Location);
}

void UMinimapGlobal::RemoveTempPin(AController* Controller)
{
	if (!IsValid(Controller) || Controller->GetWorld() != GetWorld()) return;
	if (!GetOwner()->HasAuthority())
	{
		if (auto* Player = GetRequestingPlayer(this, Controller)) Player->ServerRemoveTempPin();
		return;
	}
	AMapPinActor* Pin = nullptr;
	if (TempMapPinMapping.RemoveAndCopyValue(Controller, Pin) && IsValid(Pin)) Pin->Destroy();
}

void UMinimapGlobal::AddTempPin(AController* Controller, const FVector& Location, const FSlateBrush& Brush)
{
	if (!IsValid(Controller) || Controller->GetWorld() != GetWorld() || Location.ContainsNaN()) return;
	if (!GetOwner()->HasAuthority())
	{
		if (auto* Player = GetRequestingPlayer(this, Controller)) Player->ServerPlaceTempPin(Location, Brush);
		return;
	}
	if (auto* Existing = TempMapPinMapping.FindRef(Controller); IsValid(Existing))
	{
		Existing->SetActorLocation(Location);
		Existing->PinSlateBrush = Brush;
		if (auto* Object = Cast<UMinimapPinObject>(FindNetRelevantObject(Existing->Id)))
			Object->ReplicatingData.Brush = Brush;
		return;
	}
	const FTransform Transform(Location);
	if (auto* Pin = GetWorld()->SpawnActorDeferred<AMapPinActor>(AMapPinActor::StaticClass(), Transform,
		Controller, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Pin->OwningController = Controller;
		Pin->bCollision = true;
		Pin->PinSlateBrush = Brush;
		TempMapPinMapping.Add(Controller, Pin);
		Pin->FinishSpawning(Transform);
	}
}

void UMinimapGlobal::OnRegister()
{
	Super::OnRegister();
	PoiStateList.WorldContextObject = this;
}

FMinimapWorldSaveData UMinimapGlobal::GetSaveData() const
{
	FMinimapWorldSaveData Data;
	if (GetOwner() && GetOwner()->HasAuthority()) Data.DiscoveredPOIs = PoiStateList.PoiStateMap;
	return Data;
}

void UMinimapGlobal::LoadGame(const FMinimapWorldSaveData& Data)
{
	if (GetOwner() && GetOwner()->HasAuthority()) PoiStateList.ReplaceWith(Data.DiscoveredPOIs);
}

void UMinimapGlobal::BeginPlay()
{
	Super::BeginPlay();

	// Broadcast this to make user easily to know the data source component is ready.
	if (const auto Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>())
	{
		Sub->OnMinimapGlobalReadyEvent.Broadcast(this);
	}
}

void UMinimapGlobal::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateLocalMapPins();
	if (GetOwner()->HasAuthority())
	{
		for (int i = 0; i < NetRelevantObjects.Objects.Num(); ++i)
		{
			// Check object is valid(not valid is not allow, it should be error.)
			const auto Object = NetRelevantObjects.Objects[i].Object;
			if (!Object)
			{
				UE_LOG(LogMinimap, Error, TEXT("Object index : %d is not valid during ticking object."), i)
				continue;
			}
			
			// Groups should be valid.
			const auto View = UNetRelevantObjectFunctionLibrary::GetObjectNetGroupsView(Object);
			if (View.Num() > 0)
			{
				if (const auto PinObject = Cast<UMinimapPinObject>(Object))
				{
					PinObject->UpdateTransform();
				}
			}
		}
	}
}

void UMinimapGlobal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UMinimapGlobal::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PoiStateList)
}
