// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapBlueprintFunctionLibrary.h"
#include "GameFramework/PlayerController.h"

TMap<FString, TSoftObjectPtr<UMinimapMapData>> UMinimapBlueprintFunctionLibrary::GetMinimapDatas()
{
    if (UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>())
    {
        return Settings->MapsInfos;
    }
    TMap<FString, TSoftObjectPtr<UMinimapMapData>> Empty;
    return Empty;
}

TSoftObjectPtr<UMinimapMapData> UMinimapBlueprintFunctionLibrary::GetMinimapDataByName(const FString& LevelName)
{
    if (GetMinimapDatas().Find(LevelName))
    {
        return GetMinimapDatas().Find(LevelName)->Get();
    }
    return nullptr;
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

bool UMinimapBlueprintFunctionLibrary::ProjectWorldToScreenBidirectional(APlayerController const* Player,
    const FVector& WorldPosition, FVector2D& ScreenPosition, bool& bTargetBehindCamera, bool bPlayerViewportRelative)
{
    FVector Projected;
    bool bSuccess = false;

    ULocalPlayer* const LP = Player ? Player->GetLocalPlayer() : nullptr;
    if (LP && LP->ViewportClient)
    {
        // get the projection data
        FSceneViewProjectionData ProjectionData;
        if (LP->GetProjectionData(LP->ViewportClient->Viewport, /*out*/ ProjectionData))
        {
            const FMatrix ViewProjectionMatrix = ProjectionData.ComputeViewProjectionMatrix();
            const FIntRect ViewRectangle = ProjectionData.GetConstrainedViewRect();

            FPlane Result = ViewProjectionMatrix.TransformFVector4(FVector4(WorldPosition, 1.f));
            if (Result.W < 0.f) { bTargetBehindCamera = true; } else { bTargetBehindCamera = false; }
            if (Result.W == 0.f) { Result.W = 1.f; } // Prevent Divide By Zero

            const float RHW = 1.f / FMath::Abs(Result.W);
            Projected = FVector(Result.X, Result.Y, Result.Z) * RHW;

            // Normalize to 0..1 UI Space
            const float NormX = (Projected.X / 2.f) + 0.5f;
            const float NormY = 1.f - (Projected.Y / 2.f) - 0.5f;

            Projected.X = (float)ViewRectangle.Min.X + (NormX * (float)ViewRectangle.Width());
            Projected.Y = (float)ViewRectangle.Min.Y + (NormY * (float)ViewRectangle.Height());

            bSuccess = true;
            ScreenPosition = FVector2D(Projected.X, Projected.Y);

            if (bPlayerViewportRelative)
            {
                ScreenPosition -= FVector2D(ProjectionData.GetConstrainedViewRect().Min);
            }
        }
        else
        {
            ScreenPosition = FVector2D(1234, 5678);
        }
    }

    return bSuccess;
}

TSubclassOf<UUserWidget> UMinimapBlueprintFunctionLibrary::GetMinimapWidgetClass()
{
    if (UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>())
    {
        return Settings->GetMinimapWidgetClass();
    }
    return nullptr;
}

TSubclassOf<UUserWidget> UMinimapBlueprintFunctionLibrary::GetMainmapWidgetClass()
{
    if (UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>())
    {
        return Settings->GetMainmapWidgetClass();
    }
    return nullptr;
}
