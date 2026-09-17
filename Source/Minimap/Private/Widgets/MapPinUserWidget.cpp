// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/MapPinUserWidget.h"

#include "Components/MinimapGlobal.h"
#include "GameFramework/GameStateBase.h"
#include "Widgets/MinimapWidgetInterface.h"

FSlateBrush UMapPinUserWidget::GetMapPinBrush()
{
	FSlateBrush Brush;
	
	if (const auto Interface = TryGetInterface())
	{
		Interface->GetBrush(Guid, Brush);
	}
	
	return Brush;
}

IMinimapWidgetInterface* UMapPinUserWidget::TryGetInterface() const
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
