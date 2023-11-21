// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapSettings.h"
#include <Kismet/BlueprintPathsLibrary.h>

#include "AssetViewUtils.h"

UMinimapSettings::UMinimapSettings(const FObjectInitializer& obj)
{
    TArray<FMinimapStruct> emptyData;
    MapsData = emptyData;
    MapTexturePath = TEXT("/Game/MapTextures/");
    UniqueColors = {FLinearColor::Blue, FLinearColor::Green, FLinearColor::Red, FLinearColor::Yellow};
    ControllerHitResultDistance = 1000000.f;
}
