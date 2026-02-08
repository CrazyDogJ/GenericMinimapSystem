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
	const auto Class = GetCustomClass(Guid);
	if (Class && GetMarkersOverlay())
	{
		WidgetPool.GetOrCreateInstance<UMapPinUserWidget>(Class,
			[this, Guid](UUserWidget* WidgetObject, const TSharedRef<SWidget>& Content)
			{
				if (UMapPinUserWidget* NewMapPin = Cast<UMapPinUserWidget>(WidgetObject))
				{
					NewMapPin->Guid = Guid;
					GetMarkersOverlay()->AddChildToOverlay(NewMapPin);
					Markers.Add(Guid, NewMapPin);
				}
				
				return SNew(SObjectWidget, WidgetObject)[Content];
			});
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
	return GetWorld()->GetSubsystem<UMinimapSubsystem>();
}

AActor* UMinimapBaseUserWidget::GetLocalPlayerActor() const
{
	if (const auto MinimapSubsystem = GetMinimapSubsystem())
	{
		return MinimapSubsystem->CurrentLocalPlayerActor;
	}
	
	return nullptr;
}

UMinimapComponent_Player* UMinimapBaseUserWidget::GetLocalPlayerMinimapComponent() const
{
	if (const auto LocalPlayerActor = GetLocalPlayerActor())
	{
		return LocalPlayerActor->GetComponentByClass<UMinimapComponent_Player>();
	}

	return nullptr;
}
