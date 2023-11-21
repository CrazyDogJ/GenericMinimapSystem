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
	static TArray<FMinimapStruct> GetMinimapDatas();

	UFUNCTION(BlueprintPure, Category = "Minimap")
	static FMinimapStruct GetMinimapDataByName(const FString& LevelName);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	static FHitResult GetHitResultFromScreenPosition(const APlayerController* PlayerController, const FVector2D ScreenPosition);

	UFUNCTION(BlueprintPure, Category = "Minimap")
	static FLinearColor GetUniqueColorByIndex(const int32 index);
};
