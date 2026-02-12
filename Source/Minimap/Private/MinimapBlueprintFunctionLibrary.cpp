// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapBlueprintFunctionLibrary.h"

#include "Actors/MapPinActor.h"
#include "Components/MinimapComponent_Player.h"
#include "MinimapUserSettings.h"
#include "Blueprint/WidgetTree.h"
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

TSubclassOf<UMinimapUserWidget> UMinimapBlueprintFunctionLibrary::GetMinimapWidgetClass()
{
    if (UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>())
    {
        return Settings->GetMinimapWidgetClass();
    }
    return nullptr;
}

TSubclassOf<UMainMapUserWidget> UMinimapBlueprintFunctionLibrary::GetMainmapWidgetClass()
{
    if (UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>())
    {
        return Settings->GetMainmapWidgetClass();
    }
    return nullptr;
}

UMinimapUserSettings* UMinimapBlueprintFunctionLibrary::GetMinimapUserSettings()
{
    if (UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>())
    {
        if (auto UserSettingsClass = Settings->GetMinimapUserSettingsClass())
        {
            return NewObject<UMinimapUserSettings>(Settings, UserSettingsClass);
        }
    }
    return nullptr;
}

UWidget* UMinimapBlueprintFunctionLibrary::FindParentWidgetOfType(UWidget* StartingWidget, TSubclassOf<UWidget> Type)
{
    while ( StartingWidget )
    {
        UWidget* LocalRoot = StartingWidget;
        UWidget* LocalParent = LocalRoot->GetParent();
        while (LocalParent)
        {
            if (LocalParent->IsA(Type))
            {
                return LocalParent;
            }
            LocalRoot = LocalParent;
            LocalParent = LocalParent->GetParent();
        }

        UWidgetTree* WidgetTree = Cast<UWidgetTree>(LocalRoot->GetOuter());
        if ( WidgetTree == nullptr )
        {
            break;
        }

        StartingWidget = Cast<UUserWidget>(WidgetTree->GetOuter());
        if ( StartingWidget && StartingWidget->IsA(Type) )
        {
            return StartingWidget;
        }
    }

    return nullptr;
}

void UMinimapBlueprintFunctionLibrary::UpdateNavQueryEndPoint(APlayerController* PlayerController)
{
    if (!PlayerController)
    {
        return;
    }
    
    if (!PlayerController->GetPawn())
    {
        return;
    }

    if (const auto LocalPlayerComp = PlayerController->GetPawn()->GetComponentByClass<UMinimapComponent_Player>())
    {
        if (const auto Subsystem = PlayerController->GetLocalPlayer()->GetSubsystem<UMinimapSubsystem>())
        {
            if (LocalPlayerComp->TempPin)
            {
                Subsystem->NavQueryEndPosition = LocalPlayerComp->TempPin->GetActorLocation();
                Subsystem->bShouldUpdateNavQuery = true;
            }
            else
            {
                Subsystem->bShouldUpdateNavQuery = false;
            }
        }
    }
}
