// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MinimapBaseUserWidget.h"

#include "MinimapSubsystem.h"
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
		MapPin->RemoveFromParent();
		Markers.Remove(Guid);
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
	
	if (const auto GS = GetWorld()->GetGameState())
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
	return GetWorld()->GetSubsystem<UMinimapSubsystem>();
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
