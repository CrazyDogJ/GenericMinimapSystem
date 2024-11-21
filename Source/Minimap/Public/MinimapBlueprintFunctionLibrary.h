// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MinimapSettings.h"
#include "MinimapBlueprintFunctionLibrary.generated.h"

class APlayerController;
/**
 * 
 */
UCLASS()
class MINIMAP_API UMinimapBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "Minimap")
	static TMap<FString, TSoftObjectPtr<UMinimapMapData>> GetMinimapDatas();

	UFUNCTION(BlueprintPure, Category = "Minimap")
	static TSoftObjectPtr<UMinimapMapData> GetMinimapDataByName(const FString& LevelName);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	static FHitResult GetHitResultFromScreenPosition(const APlayerController* PlayerController, const FVector2D ScreenPosition);

	UFUNCTION(BlueprintPure, Category = "Minimap")
	static FLinearColor GetUniqueColorByIndex(const int32 index);

	UFUNCTION(BlueprintPure, Category = "Camera")
	static bool ProjectWorldToScreenBidirectional(APlayerController const* Player, const FVector& WorldPosition, FVector2D& ScreenPosition, bool& bTargetBehindCamera, bool bPlayerViewportRelative = false);

	UFUNCTION(BlueprintPure, Category = "Widget")
	static TSubclassOf<UUserWidget> GetMinimapWidgetClass();

	UFUNCTION(BlueprintPure, Category = "Widget")
	static TSubclassOf<UUserWidget> GetMainmapWidgetClass();
};
