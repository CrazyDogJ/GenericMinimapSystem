// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MinimapBaseUserWidget.h"

#include "MinimapSubsystem.h"
#include "Components/MinimapComponent_Player.h"
#include "Components/Overlay.h"
#include "Widgets/MapPinUserWidget.h"

void UMinimapBaseUserWidget::AddMapPin(FGuid Guid)
{
	if (const auto Class = GetCustomClass(Guid))
	{
		UMapPinUserWidget* NewMapPin = CreateWidget<UMapPinUserWidget>(GetOwningPlayer(), Class);
		NewMapPin->Guid = Guid;

		GetMarkersOverlay()->AddChildToOverlay(NewMapPin);
		Markers.Add(Guid, NewMapPin);
	}
}

void UMinimapBaseUserWidget::RemoveMapPin(FGuid Guid)
{
	if (const auto Found = Markers.Find(Guid))
	{
		UMapPinUserWidget* MapPin = *Found;
		MapPin->RemoveFromParent();
		Markers.Remove(Guid);
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
