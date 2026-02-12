// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapUserSettings.h"

#include "MinimapBlueprintFunctionLibrary.h"
#include "MinimapSubsystem.h"

void UMinimapUserSettings::K2_SaveConfig(UObject* WorldContextObject)
{
	SaveConfig();
	if (!WorldContextObject)
	{
		return;
	}

	const auto GI = WorldContextObject->GetWorld()->GetGameInstance();
	UMinimapBlueprintFunctionLibrary::ForEachLocalPlayerSubsystem
	<UMinimapSubsystem>(GI, [this](UMinimapSubsystem* Subsystem)
	{
		Subsystem->OnMinimapUserSettingsChangedEvent.Broadcast(this);
	});
}
