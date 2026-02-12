// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MinimapBaseUserWidget.h"

#include "MinimapSubsystem.h"
#include "Components/MinimapComponent_Player.h"
#include "Components/Overlay.h"
#include "Widgets/MapPinUserWidget.h"

void UMinimapBaseUserWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	WidgetPool.ReleaseAllSlateResources();
	
	Super::ReleaseSlateResources(bReleaseChildren);
}

UMinimapBaseUserWidget::UMinimapBaseUserWidget(const FObjectInitializer& Initializer)
	: Super(Initializer), WidgetPool(*this)
{
}

void UMinimapBaseUserWidget::AddMapPin(FGuid Guid)
{
	bool bConstructCalled = false;
	const auto Function =
		[this, Guid, &bConstructCalled](UUserWidget* WidgetObject, const TSharedRef<SWidget>& Content)
		{
			if (UMapPinUserWidget* NewMapPin = Cast<UMapPinUserWidget>(WidgetObject))
			{
				NewMapPin->Guid = Guid;
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

UMinimapSubsystem* UMinimapBaseUserWidget::GetMinimapSubsystem() const
{
	return GetOwningPlayer()->GetLocalPlayer()->GetSubsystem<UMinimapSubsystem>();
}

AActor* UMinimapBaseUserWidget::GetLocalPlayerActor() const
{
	return GetOwningPlayerPawn();
}

UMinimapComponent_Player* UMinimapBaseUserWidget::GetLocalPlayerMinimapComponent() const
{
	if (const auto LocalPlayerActor = GetLocalPlayerActor())
	{
		return LocalPlayerActor->GetComponentByClass<UMinimapComponent_Player>();
	}

	return nullptr;
}
