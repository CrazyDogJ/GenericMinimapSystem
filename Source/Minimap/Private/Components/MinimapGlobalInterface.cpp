// Fill out your copyright notice in the Description page of Project Settings.

#include "MinimapPinObject.h"
#include "MinimapMapData.h"
#include "Components/MinimapPlayerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
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

bool UMinimapGlobal::IsPoiVisible(const APlayerController* Viewer, const FString& Level, FGuid Id) const
{
	if (PoiStateList.IsPoiFound(Level, Id)) return true;
	if (!Viewer || Viewer->GetWorld() != GetWorld()) return false;
	const auto* Player = Viewer->GetComponentByClass<UMinimapPlayerComponent>();
	return Player && Player->PersonalPoiStateList.IsPoiFound(Level, Id);
}

void UMinimapGlobal::GetFoundPoisFor(const APlayerController* Viewer, const FString& Level, TSet<FGuid>& Out) const
{
	if (const auto* Shared = PoiStateList.PoiStateMap.Find(Level)) Out.Append(Shared->Indices);
	if (Viewer && Viewer->GetWorld() == GetWorld())
	{
		if (const auto* Player = Viewer->GetComponentByClass<UMinimapPlayerComponent>())
		{
			if (const auto* Personal = Player->PersonalPoiStateList.PoiStateMap.Find(Level)) Out.Append(Personal->Indices);
		}
	}
}

void UMinimapGlobal::QueryPoisFor(const APlayerController* Viewer, UMinimapMapData* Map, const FVector& Center,
	float Radius, TSet<FGuid>& Out) const
{
	if (!Map) return;
	const FString Level = Map->LevelName.IsEmpty() ? UGameplayStatics::GetCurrentLevelName(this) : Map->LevelName;
	for (const FGuid& Id : Map->QueryRange(Center, Radius))
		if (IsPoiVisible(Viewer, Level, Id)) Out.Add(Id);
}

bool UMinimapGlobal::GetFoundHotPoints(TSet<FGuid>& OutGuid) const
{
	const auto* Instance = GetWorld()->GetGameInstance();
	auto* Map = GetWorld()->GetSubsystem<UMinimapSubsystem>()->GetCurrentMinimapMapData();
	GetFoundPoisFor(Instance ? Instance->GetFirstLocalPlayerController() : nullptr,
		Map && !Map->LevelName.IsEmpty() ? Map->LevelName : UGameplayStatics::GetCurrentLevelName(this), OutGuid);
	return true;
}

bool UMinimapGlobal::QueryHotPoints(const FVector& Location, const float& Radius, TSet<FGuid>& OutGuid) const
{
	const auto* Instance = GetWorld()->GetGameInstance();
	auto* Map = GetWorld()->GetSubsystem<UMinimapSubsystem>()->GetCurrentMinimapMapData();
	QueryPoisFor(Instance ? Instance->GetFirstLocalPlayerController() : nullptr, Map, Location, Radius, OutGuid);
	return Map != nullptr;
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
	
	FPoiInfo Info;
	if (auto* Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>(); Sub && Sub->GetHotPointInfoFromGuid(Id, Info))
		return Info.bAlwaysOnMinimap;
	return IMinimapWidgetInterface::GetIsAlwaysOnMinimap(Id);
}

bool UMinimapGlobal::GetIsOnOverlay(const FGuid& Id) const
{
	if (const auto Found = LocalNetRelevantObjects.Find(Id))
	{
		const auto Object = *Found;
		if (const auto MapPinObject = Cast<UMinimapPinObject>(Object))
		{
			return MapPinObject->ReplicatingData.bAddToOverlay;
		}
	}
	else if (const auto MinimapData = LocalMapPins.Find(Id))
	{
		return MinimapData->bAddToOverlay;
	}
	
	FPoiInfo Info;
	if (auto* Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>(); Sub && Sub->GetHotPointInfoFromGuid(Id, Info))
		return Info.bAddToOverlay;
	return IMinimapWidgetInterface::GetIsOnOverlay(Id);
}

void UMinimapGlobal::GetNameAndDescription(const FGuid& Id, FText& OutName, FText& OutDesc) const
{
	if (const auto Found = LocalNetRelevantObjects.Find(Id))
	{
		const auto Object = *Found;
		if (const auto MapPinObject = Cast<UMinimapPinObject>(Object))
		{
			OutName = MapPinObject->ReplicatingData.DisplayName;
			OutDesc = MapPinObject->ReplicatingData.DisplayDescription;
		}
	}
	else if (const auto MinimapData = LocalMapPins.Find(Id))
	{
		OutName = MinimapData->DisplayName;
		OutDesc = MinimapData->DisplayDescription;
	}
	
	else
	{
		FPoiInfo Info;
		if (auto* Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>(); Sub && Sub->GetHotPointInfoFromGuid(Id, Info))
		{
			OutName = Info.DisplayName;
			OutDesc = Info.DisplayDescription;
		}
	}
}

FMapPinChangeEvent* UMinimapGlobal::GetMapPinAddEvent()
{
	return &OnMapPinAddEvent;
}

FMapPinChangeEvent* UMinimapGlobal::GetMapPinRemoveEvent()
{
	return &OnMapPinRemoveEvent;
}
