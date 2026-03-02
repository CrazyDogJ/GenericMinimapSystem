// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapBaseUserWidget.h"
#include "Components/MinimapComponent.h"
#include "MinimapStructs.h"
#include "MainMapUserWidget.generated.h"

class UMapPinUserWidget;
class UOverlay;
class UMinimapMapData;
class UImage;
class UMinimapSubsystem;
class UMinimapComponent_Player;
class UTextBlock;
class USlider;
/**
 * 
 */
UCLASS()
class MINIMAP_API UMainMapUserWidget : public UMinimapBaseUserWidget
{
	GENERATED_BODY()

protected:
	UFUNCTION()
	void OnValueChanged(float Value);
	UFUNCTION()
	void OnStaticReg(const FStaticMapPin& StaticMapPin);
	UFUNCTION()
	void OnStaticUnreg(const FStaticMapPin& StaticMapPin);
	UFUNCTION()
	void OnCompReg(UMinimapComponent* Component);
	UFUNCTION()
	void OnCompUnreg(UMinimapComponent* Component);
	UFUNCTION()
	void OnHotPointFound(const FHotPointInfo& HotPointInfo);

	void UpdateText() const;
	void UpdateDragging();
	void UpdateTransform() const;
	void UpdateMarkers();
	
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	virtual TSubclassOf<UMapPinUserWidget> GetCustomClass(const FGuid& Guid) override;

	// Mouse Input
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	// Mouse Input
	
	void ManageEvents(bool bManage);
	
public:
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
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap|Widgets")
	USlider* GetSliderWidget() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap|Widgets")
	UTextBlock* GetScaleDisplayText() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap|Widgets")
	UWidget* GetMapRootWidget() const;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap|Widgets")
	UImage* GetImageWidget() const;

	UFUNCTION(BlueprintNativeEvent, Category = "Minimap|Widgets")
	bool IsMapPinVisible(FGuid Guid);

	UFUNCTION(BlueprintNativeEvent, Category = "Minimap|Widgets")
	FText GetDisplayScaleText() const;
	
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
