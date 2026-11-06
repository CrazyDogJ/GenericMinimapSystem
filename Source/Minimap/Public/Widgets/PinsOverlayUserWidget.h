// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapBaseUserWidget.h"
#include "PinsOverlayUserWidget.generated.h"

UCLASS()
class MINIMAP_API UPinsOverlayUserWidget : public UMinimapBaseUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TSubclassOf<UMapPinUserWidget> MapPinUserWidgetClass;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	float RadiusMultiplier = 1.0f;
	
	UFUNCTION(BlueprintImplementableEvent)
	UWidget* GetPinArrowWidget(const UMapPinUserWidget* PinWidget) const;
	
	void OnMapPinAddEvent(const FGuid& Guid);
	void OnMapPinRemoveEvent(const FGuid& Guid);
	
	void ManageEvents(const bool ManageEvents);
	
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual void NativeDestruct() override;
	
	virtual TSubclassOf<UMapPinUserWidget> GetCustomClass(const FGuid& Guid) override;
};
