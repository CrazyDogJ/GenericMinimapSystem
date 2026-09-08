// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapSettings.generated.h"

class UMinimapMapData;
class UMinimapUserSettings;
class UMainMapUserWidget;
class UMinimapUserWidget;

/**
 * 
 */
UCLASS(config = MinimapSetting, defaultconfig, notplaceable)
class MINIMAP_API UMinimapSettings : public UObject
{
	GENERATED_BODY()
	
public:
	UMinimapSettings(const FObjectInitializer& obj);

	/** Store map data by level name
	 * May have problem in case that different levels have same name!
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Minimap Settings")
	TMap<FString, TSoftObjectPtr<UMinimapMapData>> MapsInfos;

	/** Where to store map textures. */
	UPROPERTY(Config, EditAnywhere, Category = "Minimap Settings")
	FString MapTexturePath;

	/** Unique color for multiplayer
	 * if not found, it will be purple.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Minimap Settings")
	TArray<FLinearColor> UniqueColors;

	/** Add temp pin the farthest line trace distance, also used in main map. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Minimap Settings")
	float ControllerHitResultDistance;

	/** Mini map widget class */
	UPROPERTY(EditAnywhere, config, meta = (MetaClass = "/Script/Minimap.MinimapUserWidget"), AdvancedDisplay, Category = "Minimap Settings")
	FSoftClassPath MiniMapWidgetClass;

	/** Main map widget class */
	UPROPERTY(EditAnywhere, config, meta = (MetaClass = "/Script/Minimap.MainMapUserWidget"), AdvancedDisplay, Category = "Minimap Settings")
	FSoftClassPath MainMapWidgetClass;

	UPROPERTY(EditAnywhere, Config, AdvancedDisplay, Category = "Minimap Settings")
	TSoftClassPtr<UMinimapUserSettings> MinimapUserSettingsClass;

	UPROPERTY(EditAnywhere, Config, Category = "Navigation")
	float NavQueryPeriod = 0.2f;

	UPROPERTY(EditAnywhere, Config, Category = "Navigation")
	float HeuristicScale = 1.f;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Config, Category = "Quad Tree")
	int32 MaxPOIPerNode = 10;

	UPROPERTY(EditAnywhere, Config, Category = "Quad Tree")
	int32 MaxDepth = 6;
#endif
	
	/** Get mini map widget class */
	TSubclassOf<UMinimapUserWidget> GetMinimapWidgetClass() const;

	/** Get main map widget class */
	TSubclassOf<UMainMapUserWidget> GetMainmapWidgetClass() const;

	TSubclassOf<UMinimapUserSettings> GetMinimapUserSettingsClass() const;
};
