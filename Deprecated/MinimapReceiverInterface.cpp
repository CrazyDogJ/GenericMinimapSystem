// Fill out your copyright notice in the Description page of Project Settings.

#include "MinimapMapData.h"
#include "MinimapSubsystem.h"
#include "Components/MinimapReceiver.h"

bool UMinimapReceiver::IsLocalPlayer(const APawn* InPawn, const FGuid& Id) const
{
	// TODO : Check guid is locally controlled.
	return IMinimapWidgetInterface::IsLocalPlayer(InPawn, Id);
}

bool UMinimapReceiver::GetRegisteredMapPins(TSet<FGuid>& OutGuid) const
{
	TArray<FGuid> Guids;
	ReplicatedMapPinList.QueryMapping.GetKeys(Guids);
	TArray<FGuid> LocalGuids;
	LocalMapPinList.QueryMapping.GetKeys(LocalGuids);
	Guids.Append(LocalGuids);
	OutGuid = TSet<FGuid>(Guids);
	return true;
}

bool UMinimapReceiver::GetFoundHotPoints(TSet<FGuid>& OutGuid) const
{
	// TODO : Found hot point record in receiver.
	return IMinimapWidgetInterface::GetFoundHotPoints(OutGuid);
}

bool UMinimapReceiver::QueryHotPoints(const FVector& Location, const float& Radius, TSet<FGuid>& OutGuid) const
{
	if (const auto Sub = GetMinimapSubsystem())
	{
		if (const auto CurrentMinimapData = Sub->GetCurrentMinimapMapData())
		{
			TArray<FGuid> Guids = CurrentMinimapData->QueryRange(Location, Radius);
			// OutGuid = TSet<FGuid>(Guids);
			// TODO : check hot point is found.
		}
	}
	
	return true;
}

bool UMinimapReceiver::GetMapPinClass(const FGuid& Id, const uint8 Type, TSubclassOf<UMapPinUserWidget>& OutClass) const
{
	
	return IMinimapWidgetInterface::GetMapPinClass(Id, Type, OutClass);
}

bool UMinimapReceiver::GetLocation(const FGuid& Id, FVector& OutLocation) const
{
	return IMinimapWidgetInterface::GetLocation(Id, OutLocation);
}

bool UMinimapReceiver::GetYaw(const FGuid& Id, float& OutYaw) const
{
	return IMinimapWidgetInterface::GetYaw(Id, OutYaw);
}

bool UMinimapReceiver::GetBrush(const FGuid& Id, FSlateBrush& OutBrush) const
{
	return IMinimapWidgetInterface::GetBrush(Id, OutBrush);
}

bool UMinimapReceiver::GetCategoryTag(const FGuid& Id, FGameplayTag& OutTag) const
{
	return IMinimapWidgetInterface::GetCategoryTag(Id, OutTag);
}

bool UMinimapReceiver::GetIsAlwaysOnMinimap(const FGuid& Id) const
{
	return IMinimapWidgetInterface::GetIsAlwaysOnMinimap(Id);
}

FMapPinChangeEvent* UMinimapReceiver::GetMapPinAddEvent()
{
	return IMinimapWidgetInterface::GetMapPinAddEvent();
}

FMapPinChangeEvent* UMinimapReceiver::GetMapPinRemoveEvent()
{
	return IMinimapWidgetInterface::GetMapPinRemoveEvent();
}

void UMinimapReceiver::AddTempPin(const FVector2D Location, const UMinimapMapData* MapData,
	const ECollisionChannel TraceChannel)
{
}
