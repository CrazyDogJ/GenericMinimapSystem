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

	if (const auto Subsystem = UWorld::GetSubsystem<UMinimapSubsystem>(WorldContextObject->GetWorld()) /* 修复：支持无世界的配置对象 */)
	{
		Subsystem->OnMinimapUserSettingsChangedEvent.Broadcast(this);
	}
}
