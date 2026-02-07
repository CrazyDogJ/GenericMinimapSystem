// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapSettings.h"
#include "Widgets/MinimapUserWidget.h"
#include "Widgets/MainMapUserWidget.h"

UMinimapSettings::UMinimapSettings(const FObjectInitializer& obj)
{
    MapTexturePath = TEXT("/Game/MapTextures/");
    UniqueColors = {FLinearColor::Blue, FLinearColor::Green, FLinearColor::Red, FLinearColor::Yellow};
    ControllerHitResultDistance = 1000000.f;
}

TSubclassOf<UMinimapUserWidget> UMinimapSettings::GetMinimapWidgetClass() const
{
    return MiniMapWidgetClass.TryLoadClass<UMinimapUserWidget>();
}

TSubclassOf<UMainMapUserWidget> UMinimapSettings::GetMainmapWidgetClass() const
{
    return MainMapWidgetClass.TryLoadClass<UMainMapUserWidget>();
}

TSubclassOf<UMinimapUserSettings> UMinimapSettings::GetMinimapUserSettingsClass() const
{
    return MinimapUserSettingsClass.LoadSynchronous();
}
