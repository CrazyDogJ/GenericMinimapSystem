// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MapPinUserWidget.h"

#include "MinimapSubsystem.h"

bool UMapPinUserWidget::GetMapPinState(FMapPinStateEntry& OutMapPinState) const
{
	if (GetWorld())
	{
		if (const auto Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>())
		{
			return Sub->GetMapPinCurrentState(Guid, OutMapPinState);
		}
	}
	
	return false;
}
