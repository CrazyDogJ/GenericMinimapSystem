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

UCLASS()
class MINIMAP_API UMinimapUserWidget : public UMinimapBaseUserWidget
{
	GENERATED_BODY()
	
public:
	UMinimapUserWidget(const FObjectInitializer& ObjectInitializer);
	
protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	
	static bool IsInRadius(const FVector& CenterLocation, const FVector& CheckLocation, const float& Radius);
	bool IsInRadius(const FVector& CenterLocation, const FVector& CheckLocation) const;
	bool IsInRadius(const FVector& CenterLocation, const FGuid& InMapPinId) const;
	bool IsInRadius(const FGuid& InMapPinId) const;
	
	void InitializeRadius();
	void InitializeMapPins();
	void ManagerEvents(const bool& BindOrUnbind);
	
	void OnMapPinAdd(const FGuid& Guid);
	void OnMapPinRemove(const FGuid& Guid);

	void UpdateInterpRadius(const float DeltaTime);
	void UpdateViewAngle() const;
	void UpdateLocalPlayerAngle() const;
	void UpdateNorthWidgets() const;
	void UpdateMarker(const IMinimapWidgetInterface* Interface, const FGuid& Id, UMapPinUserWidget* Widget) const;
	void UpdateMarker(UMapPinUserWidget* MapPin, FGameplayTag CategoryTag, FVector2D WorldPosition2D, float Angle, bool bRotate = true) const;
	void UpdateMarkers() const;
	void UpdateMinimapImageParameters() const;
	
	void UpdateMarkersVisibilities();
	
public:
#pragma region Properties
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|State")
	float InterpRadius = 0.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|State")
	float ZoomMultiplier = 1.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap|State")
	float TargetRadius = 5000.0f;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|State")
	TSet<FGuid> CachedHotPoints;
	
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
	UWidget* GetMinimapImageWidget() const;
	
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
	float GetViewAngle() const;

	/** Return player pawn actor rotation. */
	UFUNCTION(BlueprintNativeEvent, Category = "Minimap")
	FTransform GetLocalPlayerTransform() const;
	
	UFUNCTION(BlueprintNativeEvent, Category = "Minimap")
	float GetDesiredRadius() const;
#pragma endregion
#pragma region Functions
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMinimapMapData* GetCurrentMapData() const;

	UFUNCTION(BlueprintPure, Category = "Minimap")
	FVector2D GetWidgetPosition(const FVector2D InWorldPosition2D) const;

	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsLocalPlayerMarker(const FGuid Guid) const;

	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsCurrentMapDataValid() const;
#pragma endregion
};
