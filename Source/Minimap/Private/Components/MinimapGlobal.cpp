// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/MinimapGlobal.h"

#include "Minimap.h"
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

void UMinimapGlobal::ChangeTempPinLocation_Implementation(AController* Controller, const FVector& Location)
{
	if (const auto Found = TempMapPinMapping.Find(Controller))
	{
		if (const auto Ptr = *Found)
		{
			Ptr->SetActorLocation(Location);
		}
	}
}

void UMinimapGlobal::RemoveTempPin_Implementation(AController* Controller)
{
	if (const auto Found = TempMapPinMapping.Find(Controller))
	{
		if (const auto Ptr = *Found)
		{
			Ptr->K2_DestroyActor();
		}
	}
	
	TempMapPinMapping.Remove(Controller);
}

void UMinimapGlobal::AddTempPin_Implementation(AController* Controller, const FVector& Location, const FSlateBrush& Brush)
{
	// If temp pin actor exist, we simply change location for the map pin actor.
	if (const auto Found = TempMapPinMapping.Find(Controller))
	{
		if (const auto Ptr = *Found)
		{
			Ptr->SetActorLocation(Location);
			return;
		}
	}
	
	FActorSpawnParameters spawnInfo;
	spawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (const auto TempPin = GetWorld()->SpawnActorDeferred<AMapPinActor>(AMapPinActor::StaticClass(), FTransform(Location)))
	{
		TempPin->OwningController = Controller;
		TempPin->bCollision = true;
		TempPin->PinSlateBrush = Brush;
		TempPin->SetOwner(GetOwner()->GetOwner());
		TempPin->FinishSpawning(FTransform(Location));
	}
}

void UMinimapGlobal::BeginPlay()
{
	PoiStateList.WorldContextObject = this;
	
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
