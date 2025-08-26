// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MinimapUserSettings.generated.h"

/**
 * 
 */
UCLASS(Config=MinimapUserSettings, Blueprintable)
class MINIMAP_API UMinimapUserSettings : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Minimap", meta=(WorldContext = "WorldContextObject"))
	void K2_SaveConfig(UObject* WorldContextObject);
};
