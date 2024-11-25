// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapPinWidget.h"
#include "MinimapMapData.h"
#include "MinimapSettings.generated.h"

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

	/** Widget class on map pin actor.
	 * Widget in plugin content folder by default.
	 */
	UPROPERTY(Config, EditAnywhere, meta = (MetaClass = "/Script/Minimap.MapPinWidget"), AdvancedDisplay, Category = "Minimap Settings")
	FSoftClassPath CommonMapPinWidget;

	/** Unique color for multiplayer
	 * if not found, it will be purple.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Minimap Settings")
	TArray<FLinearColor> UniqueColors;

	/** Add temp pin farthest line trace distance, also used in main map. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Minimap Settings")
	float ControllerHitResultDistance;

	/** Mini map widget class */
	UPROPERTY(EditAnywhere, config, meta = (MetaClass = "/Script/UMG.UserWidget"), AdvancedDisplay)
	FSoftClassPath MiniMapWidgetClass;

	/** Main map widget class */
	UPROPERTY(EditAnywhere, config, meta = (MetaClass = "/Script/UMG.UserWidget"), AdvancedDisplay)
	FSoftClassPath MainMapWidgetClass;

	/** Get mini map widget class */
	TSubclassOf<UUserWidget> GetMinimapWidgetClass() const;

	/** Get main map widget class */
	TSubclassOf<UUserWidget> GetMainmapWidgetClass() const;

	TSubclassOf<UMapPinWidget> GetCommonMapPinWidgetClass() const;
};
