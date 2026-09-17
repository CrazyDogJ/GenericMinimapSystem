// Fill out your copyright notice in the Description page of Project Settings.

#include "MinimapPinObject.h"
#include "MinimapSubsystem.h"
#include "Components/MinimapGlobal.h"
#include "Widgets/MinimapBaseUserWidget.h"

bool UMinimapGlobal::IsLocalPlayer(const APawn* InPawn, const FGuid& Id) const
{
	if (const auto Found = LocalNetRelevantObjects.Find(Id))
	{
		if (const auto Ptr = *Found)
		{
			if (const auto MapPin = Cast<UMinimapPinObject>(Ptr))
			{
				if (MapPin->ReplicatingData.AttachedComponent.IsValid())
				{
					if (const auto OwningActor = MapPin->ReplicatingData.AttachedComponent->GetOwner())
					{
						if (const auto Pawn = Cast<APawn>(OwningActor))
						{
							return Pawn->IsLocallyControlled();
						}
					}
				}
			}
		}
	}
	
	return IMinimapWidgetInterface::IsLocalPlayer(InPawn, Id);
}

bool UMinimapGlobal::GetRegisteredMapPins(TSet<FGuid>& OutGuid) const
{
	TSet<FGuid> ResultA;
	TSet<FGuid> ResultB;
	LocalNetRelevantObjects.GetKeys(ResultA);
	LocalMapPins.GetKeys(ResultB);
	OutGuid.Append(ResultA);
	OutGuid.Append(ResultB);
	return true;
}

bool UMinimapGlobal::GetFoundHotPoints(TSet<FGuid>& OutGuid) const
{
	// TODO : POI : Hot point not do it now.
	return IMinimapWidgetInterface::GetFoundHotPoints(OutGuid);
}

bool UMinimapGlobal::QueryHotPoints(const FVector& Location, const float& Radius, TSet<FGuid>& OutGuid) const
{
	// TODO : POI : Hot point not do it now.
	return IMinimapWidgetInterface::QueryHotPoints(Location, Radius, OutGuid);
}

bool UMinimapGlobal::GetMapPinClass(const FGuid& Id, const uint8 Type, TSubclassOf<UMapPinUserWidget>& OutClass) const
{
	if (const auto Found = LocalNetRelevantObjects.Find(Id))
	{
		const auto Object = *Found;
		if (const auto MapPinObject = Cast<UMinimapPinObject>(Object))
		{
			if (Type == TYPE_MAINMAP)
			{
				OutClass = MapPinObject->ReplicatingData.CustomMainmapWidgetClass;
				return true;
			}
			else if (Type == TYPE_MINIMAP)
			{
				OutClass = MapPinObject->ReplicatingData.CustomMinimapWidgetClass;
				return true;
			}
		}
	}
	else if (const auto MinimapData = LocalMapPins.Find(Id))
	{
		if (Type == TYPE_MAINMAP)
		{
			OutClass = MinimapData->CustomMainmapWidgetClass;
			return true;
		}
		else if (Type == TYPE_MINIMAP)
		{
			OutClass = MinimapData->CustomMinimapWidgetClass;
			return true;
		}
	}
	else if (const auto Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>())
	{
		FPoiInfo OutInfo;
		if (Sub->GetHotPointInfoFromGuid(Id, OutInfo))
		{
			if (Type == TYPE_MAINMAP)
			{
				OutClass = OutInfo.CustomMainmapWidgetClass;
				return true;
			}
			else if (Type == TYPE_MINIMAP)
			{
				OutClass = OutInfo.CustomMinimapWidgetClass;
				return true;
			}
		}
	}
	
	return IMinimapWidgetInterface::GetMapPinClass(Id, Type, OutClass);
}

bool UMinimapGlobal::GetLocation(const FGuid& Id, FVector& OutLocation) const
{
	if (const auto Found = LocalNetRelevantObjects.Find(Id))
	{
		const auto Object = *Found;
		if (const auto MapPinObject = Cast<UMinimapPinObject>(Object))
		{
			if (MapPinObject->ReplicatingData.AttachedComponent.IsValid())
			{
				OutLocation = MapPinObject->ReplicatingData.AttachedComponent->GetComponentLocation();
			}
			else
			{
				OutLocation = MapPinObject->ReplicatingData.Location;
			}
			return true;
		}
	}
	else if (const auto MinimapData = LocalMapPins.Find(Id))
	{
		if (MinimapData->AttachedComponent.IsValid())
		{
			OutLocation = MinimapData->AttachedComponent->GetComponentLocation();
		}
		else
		{
			OutLocation = MinimapData->Location;
		}
		return true;
	}
	else if (const auto Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>())
	{
		FPoiInfo OutInfo;
		if (Sub->GetHotPointInfoFromGuid(Id, OutInfo))
		{
			OutLocation = OutInfo.Location;
			return true;
		}
	}
	
	return IMinimapWidgetInterface::GetLocation(Id, OutLocation);
}

bool UMinimapGlobal::GetYaw(const FGuid& Id, float& OutYaw) const
{
	if (const auto Found = LocalNetRelevantObjects.Find(Id))
	{
		const auto Object = *Found;
		if (const auto MapPinObject = Cast<UMinimapPinObject>(Object))
		{
			if (MapPinObject->ReplicatingData.AttachedComponent.IsValid())
			{
				OutYaw = MapPinObject->ReplicatingData.AttachedComponent->GetComponentRotation().Yaw;
			}
			else
			{
				OutYaw = MapPinObject->ReplicatingData.Yaw;
			}
			return MapPinObject->ReplicatingData.bHasYaw;
		}
	}
	else if (const auto MinimapData = LocalMapPins.Find(Id))
	{
		if (MinimapData->AttachedComponent.IsValid())
		{
			OutYaw = MinimapData->AttachedComponent->GetComponentRotation().Yaw;
		}
		else
		{
			OutYaw = MinimapData->Yaw;
		}
		return MinimapData->bHasYaw;
	}
	else
	{
		// POI has no yaw.
		OutYaw = 0.0f;
	}
	
	return IMinimapWidgetInterface::GetYaw(Id, OutYaw);
}

bool UMinimapGlobal::GetBrush(const FGuid& Id, FSlateBrush& OutBrush) const
{
	if (const auto Found = LocalNetRelevantObjects.Find(Id))
	{
		const auto Object = *Found;
		if (const auto MapPinObject = Cast<UMinimapPinObject>(Object))
		{
			OutBrush = MapPinObject->ReplicatingData.Brush;
			return true;
		}
	}
	else if (const auto MinimapData = LocalMapPins.Find(Id))
	{
		OutBrush = MinimapData->Brush;
		return true;
	}
	else if (const auto Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>())
	{
		FPoiInfo OutInfo;
		if (Sub->GetHotPointInfoFromGuid(Id, OutInfo))
		{
			OutBrush = OutInfo.Brush;
			return true;
		}
	}
	
	return IMinimapWidgetInterface::GetBrush(Id, OutBrush);
}

bool UMinimapGlobal::GetCategoryTag(const FGuid& Id, FGameplayTag& OutTag) const
{
	if (const auto Found = LocalNetRelevantObjects.Find(Id))
	{
		const auto Object = *Found;
		if (const auto MapPinObject = Cast<UMinimapPinObject>(Object))
		{
			OutTag = MapPinObject->ReplicatingData.CategoryTag;
			return true;
		}
	}
	else if (const auto MinimapData = LocalMapPins.Find(Id))
	{
		OutTag = MinimapData->CategoryTag;
		return true;
	}
	else if (const auto Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>())
	{
		FPoiInfo OutInfo;
		if (Sub->GetHotPointInfoFromGuid(Id, OutInfo))
		{
			OutTag = OutInfo.CategoryTag;
			return true;
		}
	}
	
	return IMinimapWidgetInterface::GetCategoryTag(Id, OutTag);
}

bool UMinimapGlobal::GetIsAlwaysOnMinimap(const FGuid& Id) const
{
	if (const auto Found = LocalNetRelevantObjects.Find(Id))
	{
		const auto Object = *Found;
		if (const auto MapPinObject = Cast<UMinimapPinObject>(Object))
		{
			return MapPinObject->ReplicatingData.bAlwaysOnMinimap;
		}
	}
	else if (const auto MinimapData = LocalMapPins.Find(Id))
	{
		return MinimapData->bAlwaysOnMinimap;
	}
	
	return IMinimapWidgetInterface::GetIsAlwaysOnMinimap(Id);
}

FMapPinChangeEvent* UMinimapGlobal::GetMapPinAddEvent()
{
	return &OnMapPinAddEvent;
}

FMapPinChangeEvent* UMinimapGlobal::GetMapPinRemoveEvent()
{
	return &OnMapPinRemoveEvent;
}
