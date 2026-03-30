// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MinimapBaseUserWidget.h"
#include "MinimapUserWidget.generated.h"

class UImage;
class UOverlay;
class UMapPinUserWidget;
class UMinimapSubsystem;
class UMinimapMapData;
class UMinimapComponent_Player;

UCLASS()
class MINIMAP_API UMinimapUserWidget : public UMinimapBaseUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	virtual TSubclassOf<UMapPinUserWidget> GetCustomClass(const FGuid& Guid) override;

	UFUNCTION()
	void OnLocalMapDataChanged(const UMinimapMapData* ChangedMinimapData);

	UFUNCTION()
	void OnMarkerShowOnMinimap(FGuid Guid);

	UFUNCTION()
	void OnMarkerHideOnMinimap(FGuid Guid);

	void UpdateInterpRadius(const float DeltaTime);
	void UpdateViewAngle();
	void UpdateLocalPlayerAngle();
	void UpdateNorthWidgets();
	void UpdateMarker(UMapPinUserWidget* MapPin, FGameplayTag CategoryTag, FVector2D WorldPosition2D, float Angle, bool bRotate = true);
	void UpdateMarkers();
	void UpdateMinimapImageParameters();
	
public:
#pragma region Properties
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|State")
	float InterpRadius = 0.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|State")
	float ZoomMultiplier = 1.0f;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Minimap|Settings")
	float DefaultRadius = 5000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap|Settings")
	float InterpSpeed = 10.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap|Settings")
	float NorthYawOffset = 0.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap|Settings")
	bool bLockNorth = false;

#pragma endregion
#pragma region Implement Widgets
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap|Widgets")
	UImage* GetMinimapImageWidget() const;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap|Widgets")
	UWidget* GetViewAreaWidget() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap|Widgets")
	UWidget* GetLocalPlayerWidget() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap|Widgets")
	UWidget* GetNorthWidget() const;

	UFUNCTION(BlueprintImplementableEvent,	Category = "Minimap|Widgets")
	UWidget* GetNorthPivotWidget() const;

#pragma endregion 
#pragma region NativeEvents
	/** Return radius that minimap will display. UNIT : *cm */
	UFUNCTION(BlueprintNativeEvent, Category = "Minimap")
	float GetMinimapDisplayRadius();

	/** Return view angle. */
	UFUNCTION(BlueprintNativeEvent, Category = "Minimap")
	float GetViewAngle();

	/** Return player pawn actor rotation. */
	UFUNCTION(BlueprintNativeEvent, Category = "Minimap")
	FTransform GetLocalPlayerTransform();
	
	UFUNCTION(BlueprintNativeEvent, Category = "Minimap")
	float GetDesiredRadius();
#pragma endregion
#pragma region Functions
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMinimapMapData* GetCurrentMapData() const;

	UFUNCTION(BlueprintPure, Category = "Minimap")
	FVector2D GetWidgetPosition(const FVector2D InWorldPosition2D);

	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsLocalPlayerMarker(const FGuid Guid) const;

	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsCurrentMapDataValid() const;
#pragma endregion
};
