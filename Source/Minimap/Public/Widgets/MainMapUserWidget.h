// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapBaseUserWidget.h"
#include "MainMapUserWidget.generated.h"

class UMapPinUserWidget;
class UOverlay;
class UMinimapMapData;
class UImage;
class UMinimapSubsystem;
class UTextBlock;
class USlider;

/**
 * 
 */
UCLASS()
class MINIMAP_API UMainMapUserWidget : public UMinimapBaseUserWidget
{
	GENERATED_BODY()

public:
	UMainMapUserWidget(const FObjectInitializer& ObjectInitializer);
	
protected:
	UFUNCTION()
	void OnValueChanged(float Value);
	UFUNCTION()
	void OnMapPinAddEvent(const FGuid& MapPinId);
	UFUNCTION()
	void OnMapPinRemoveEvent(const FGuid& MapPinId);
	UFUNCTION()
	void OnHotPointFound(const FString& LevelName, const FGuid& Guid);
	UFUNCTION()
	void OnHotPointRemove(const FString& LevelName, const FGuid& Guid);

	void InitializeSlider() const;
	void InitializeMapPins();
	
	void UpdateText() const;
	void UpdateDragging();
	void UpdateTransform() const;
	void UpdateMarkers() const;
	
	void ReleaseAllMarkers();
	
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	// Mouse Input
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	// Mouse Input
	
	void ManageEvents(bool bManage);
	
	void AddTempPin(const FVector2D Location, const UMinimapMapData* MapData, const ECollisionChannel TraceChannel) const;
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap|Settings")
	bool bShowLocal = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap|Settings")
	float MinScale = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap|Settings")
	float MaxScale = 3.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap|Settings")
	FKey DragKey = EKeys::LeftMouseButton;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap|Settings")
	FKey MarkKey = EKeys::RightMouseButton;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|State")
	float Scale = 1.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|State")
	UMaterialInstanceDynamic* MaterialInstance = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|State")
	bool bMouseDragging = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|State")
	FVector2D CachedMousePosition;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap|State")
	FVector2D Offset;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap|Settings")
	FSlateBrush TempPinBrush;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap|Widgets")
	USlider* GetSliderWidget() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap|Widgets")
	UTextBlock* GetScaleDisplayText() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap|Widgets")
	UWidget* GetMapRootWidget() const;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap|Widgets")
	UWidget* GetImageWidget() const;

	UFUNCTION(BlueprintNativeEvent, Category = "Minimap|Widgets")
	bool IsMapPinVisible(FGuid Guid) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Minimap|Widgets")
	FText GetDisplayScaleText() const;
	
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsCurrentLocalMapDataValid() const;
	
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UMinimapMapData* GetCurrentGlobalMapData() const;
	
	UFUNCTION(BlueprintPure, Category = "Minimap")
	FVector GetCaptureCenter() const;

	UFUNCTION(BlueprintPure, Category = "Minimap")
	FVector2D WorldToWidget(FVector2D InWorldPosition2D, const float& InScale) const;

	UFUNCTION(BlueprintPure, Category = "Minimap")
	FVector2D WidgetToWorld(FVector2D InLocalVector2D) const;

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void LocalPawnCenter();
};
