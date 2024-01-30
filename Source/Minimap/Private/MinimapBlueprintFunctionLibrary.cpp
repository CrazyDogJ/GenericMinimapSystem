// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapBlueprintFunctionLibrary.h"
#include "GameFramework/PlayerController.h"

TArray<FMinimapStruct> UMinimapBlueprintFunctionLibrary::GetMinimapDatas()
{
    if (UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>())
    {
        return Settings->MapsData;
    }
    TArray<FMinimapStruct> empty;
    return empty;
}

FMinimapStruct UMinimapBlueprintFunctionLibrary::GetMinimapDataByName(const FString& LevelName)
{
    if (!LevelName.IsEmpty())
    {
        const TArray<FMinimapStruct> datas = GetMinimapDatas();
        for (auto data : datas)
        {
            if (data.LevelName == LevelName)
            {
                return data;
            }
        }
    }
    FMinimapStruct MinimapStruct;
    MinimapStruct.LevelName = FString();
    MinimapStruct.MapSize = 0.f;
    MinimapStruct.MapTexture = nullptr;
    MinimapStruct.TextureSize = 0.f;
    return MinimapStruct;
}

FHitResult UMinimapBlueprintFunctionLibrary::GetHitResultFromScreenPosition(const APlayerController* PlayerController, const FVector2D ScreenPosition)
{
    FHitResult result;
    PlayerController->GetHitResultAtScreenPosition(ScreenPosition, ECC_Visibility, true, result);
    return result;
}

FLinearColor UMinimapBlueprintFunctionLibrary::GetUniqueColorByIndex(const int32 index)
{
    if (UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>())
    {
        if (index < Settings->UniqueColors.Num())
        {
            return Settings->UniqueColors[index];
        }
    }
    return FLinearColor(128,0,128);
}
