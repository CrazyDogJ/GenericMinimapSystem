// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapUserSettings.h"

#include "MinimapSubsystem.h"

void UMinimapUserSettings::K2_SaveConfig(UObject* WorldContextObject)
{
	SaveConfig();
	if (!WorldContextObject)
	{
		return;
	}

	if (const auto Subsystem = WorldContextObject->GetWorld()->GetSubsystem<UMinimapSubsystem>())
	{
		Subsystem->OnMinimapUserSettingsChangedEvent.Broadcast(this);
	}
}
