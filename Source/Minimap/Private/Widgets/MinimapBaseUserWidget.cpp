// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MinimapBaseUserWidget.h"

#include "MinimapSubsystem.h"
#include "MinimapMapData.h"
#include "Kismet/GameplayStatics.h"
#include "Components/MinimapGlobal.h"
#include "Components/Overlay.h"
#include "GameFramework/GameStateBase.h"
#include "Widgets/MapPinUserWidget.h"
#include "Widgets/MinimapWidgetInterface.h"

void UMinimapBaseUserWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	WidgetPool.ReleaseAllSlateResources();
	
	Super::ReleaseSlateResources(bReleaseChildren);
}

UMinimapMapData* UMinimapBaseUserWidget::GetPoiMap() const
{
	auto* Sub = GetMinimapSubsystem();
	return Sub ? Sub->GetCurrentMinimapMapData() : nullptr;
}

void UMinimapBaseUserWidget::ShowDiscoveredPoi(FGuid Id)
{
	AddMapPin(Id);
}

void UMinimapBaseUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (auto* Sub = GetMinimapSubsystem())
	{
		Sub->OnPoiChanged.AddUObject(this, &ThisClass::OnPoiChanged);
		Sub->OnLocalMapDataChangeEvent.AddUniqueDynamic(this, &ThisClass::OnPoiMapChanged);
	}
	RefreshDiscoveredPois();
}

void UMinimapBaseUserWidget::OnPoiMapChanged(const UMinimapMapData* Map)
{
	RefreshDiscoveredPois();
}

void UMinimapBaseUserWidget::RefreshDiscoveredPois()
{
	TSet<FGuid> Current;
	if (auto* Interface = TryGetDataInterface())
	{
		if (auto* Global = Cast<UMinimapGlobal>(Interface))
		{
			const auto* Map = GetPoiMap();
			Global->GetFoundPoisFor(GetOwningPlayer(), Map && !Map->LevelName.IsEmpty() ? Map->LevelName : UGameplayStatics::GetCurrentLevelName(this), Current);
		}
		else Interface->GetFoundHotPoints(Current);
	}
	for (FGuid Id : DiscoveredPois.Difference(Current)) RemoveMapPin(Id);
	DiscoveredPois = MoveTemp(Current);
	for (FGuid Id : DiscoveredPois) ShowDiscoveredPoi(Id);
}

void UMinimapBaseUserWidget::OnPoiChanged(const APlayerController* Viewer, const FString& Level, FGuid Id)
{
	if (Viewer && Viewer != GetOwningPlayer()) return;
	auto* Interface = TryGetDataInterface();
	auto* Global = Interface ? Cast<UMinimapGlobal>(Interface) : nullptr;
	if (!Global) return;
	const auto* Map = GetPoiMap();
	if (Level != (Map && !Map->LevelName.IsEmpty() ? Map->LevelName : UGameplayStatics::GetCurrentLevelName(this))) return;
	if (Global->IsPoiVisible(GetOwningPlayer(), Level, Id))
	{
		DiscoveredPois.Add(Id);
		ShowDiscoveredPoi(Id);
	}
	else if (DiscoveredPois.Remove(Id)) RemoveMapPin(Id);
}

void UMinimapBaseUserWidget::NativeDestruct()
{
	if (auto* Sub = GetMinimapSubsystem())
	{
		Sub->OnPoiChanged.RemoveAll(this);
		Sub->OnLocalMapDataChangeEvent.RemoveAll(this);
	}
	DiscoveredPois.Reset();
	// 修复：从面板摘除标记并清空索引，再把控件交还池中。
	auto ReleasedMarkers = MoveTemp(Markers);
	for (const auto& Pair : ReleasedMarkers)
	{
		if (IsValid(Pair.Value)) Pair.Value->RemoveFromParent();
	}
	WidgetPool.ReleaseAll(true);
	
	Super::NativeDestruct();
}

UMinimapBaseUserWidget::UMinimapBaseUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), WidgetPool(*this), LocalPawn(nullptr)
{
}

void UMinimapBaseUserWidget::AddMapPin(FGuid Guid)
{
	// We do not add a map pin once more if we found it in mapping.
	if (Markers.Contains(Guid))
	{
		return;
	}
	
	bool bConstructCalled = false;
	const auto Function =
		[this, Guid, &bConstructCalled](UUserWidget* WidgetObject, const TSharedRef<SWidget>& Content)
		{
			if (UMapPinUserWidget* NewPin = Cast<UMapPinUserWidget>(WidgetObject))
			{
				NewPin->MinimapDataSourceObject = MinimapDataSourceObject;
				NewPin->Guid = Guid;
				bConstructCalled = true;
			}
			
			return SNew(SObjectWidget, WidgetObject)[Content];
		};
	
	const auto Class = GetCustomClass(Guid);
	if (Class && GetMarkersOverlay())
	{
		const auto NewPin = WidgetPool.GetOrCreateInstance<UMapPinUserWidget>(Class, Function);
		// 修复：池可能没有可用于创建控件的玩家。
		if (!NewPin) return;
		if (!bConstructCalled)
		{
			NewPin->MinimapDataSourceObject = MinimapDataSourceObject;
			NewPin->Guid = Guid;
		}
		GetMarkersOverlay()->AddChildToOverlay(NewPin);
		Markers.Add(Guid, NewPin);
	}
}

void UMinimapBaseUserWidget::RemoveMapPin(FGuid Guid)
{
	if (const auto Found = Markers.Find(Guid))
	{
		UMapPinUserWidget* MapPin = *Found;
		Markers.Remove(Guid);
		if (!IsValid(MapPin)) return;
		MapPin->RemoveFromParent();
		WidgetPool.Release(MapPin);
	}
}

TSubclassOf<UMapPinUserWidget> UMinimapBaseUserWidget::GetCustomClass(const FGuid& Guid)
{
	if (const auto Interface = TryGetDataInterface())
	{
		TSubclassOf<UMapPinUserWidget> Result;
		if (Interface->GetMapPinClass(Guid, MarkerClassType, Result))
		{
			if (Result)
			{
				return Result;
			}
		}
	}
	
	return MarkerWidgetClass;
}

IMinimapWidgetInterface* UMinimapBaseUserWidget::TryGetDataInterface() const
{
	if (MinimapDataSourceObject.IsValid())
	{
		return Cast<IMinimapWidgetInterface>(MinimapDataSourceObject.Get());
	}
	
	if (const auto GS = GetWorld() ? GetWorld()->GetGameState() : nullptr)
	{
		if (const auto Global = GS->GetComponentByClass<UMinimapGlobal>())
		{
			return Cast<IMinimapWidgetInterface>(Global);
		}
	}
	
	return nullptr;
}

bool UMinimapBaseUserWidget::ShouldHide(const FGuid& Id) const
{
	if (const auto Interface = TryGetDataInterface())
	{
		FGameplayTag Tag;
		if (Interface->GetCategoryTag(Id, Tag))
		{
			return ShouldHide(Tag);
		}
	}
	
	return false;
}

bool UMinimapBaseUserWidget::ShouldHide(const FGameplayTag& InTag) const
{
	return HiddenCategoryTags.Contains(InTag);
}

UMinimapSubsystem* UMinimapBaseUserWidget::GetMinimapSubsystem() const
{
	return UWorld::GetSubsystem<UMinimapSubsystem>(GetWorld());
}

AActor* UMinimapBaseUserWidget::GetLocalPlayerActor() const
{
	if (LocalPawn)
	{
		return LocalPawn;
	}
	else
	{
		return GetOwningPlayerPawn();
	}
}
