// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "ZoomableCanvas.generated.h"

class UImage;
class SZoomableCanvas;

UCLASS()
class MINIMAP_API UZoomableCanvas : public UWidget
{
	GENERATED_BODY()
	
public:
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Appearance)
	bool bEnableInput = false;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Appearance)
	bool bDebug = false;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Appearance)
	float ViewScale = 1.0f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Appearance)
	FVector2D ViewOffset = FVector2D(0.0f, 0.0f);
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Appearance)
	float ZoomStep = 0.1f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Appearance)
	float MinScale = 0.1f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Appearance)
	float MaxScale = 5.0f;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Appearance)
	float DebugMultiplier = 1.0f;
	
	UFUNCTION(BlueprintCallable, Category = Appearance)
	void SetViewScale(float Scale);
	
	UFUNCTION(BlueprintCallable, Category = Appearance)
	void SetViewOffset(FVector2D Offset);
	
	UFUNCTION(BlueprintCallable, Category = Appearance)
	void SetEnableInput(bool bNewEnableInput);
	
	UFUNCTION(BlueprintCallable, Category = Appearance)
	void SetDebugEnabled(bool Enable);
	
	UFUNCTION(BlueprintCallable, Category = Appearance)
	void SetZoomStep(float InZoomStep);
	
	UFUNCTION(BlueprintPure, Category = Appearance)
	float GetCurrentScale() const;
	
	UFUNCTION(BlueprintCallable, Category = Appearance)
	void SetMinScale(float Scale);
	
	UFUNCTION(BlueprintCallable, Category = Appearance)
	void SetMaxScale(float Scale);
	
	UFUNCTION(BlueprintCallable, Category = Appearance)
	void SetDebugMultiplier(float Multiplier);
	
	UFUNCTION(BlueprintCallable)
	void AddWidgetToCanvas(UWidget* InWidget, FVector2D InPosition, FVector2D InSize) const;
	
#if WITH_EDITOR 
	virtual const FText GetPaletteCategory() override;
#endif
	
protected:
	TSharedPtr<SZoomableCanvas> MyZoomableCanvas;
	
	virtual TSharedRef<SWidget> RebuildWidget() override;
	
#if WITH_ACCESSIBILITY 
	virtual TSharedPtr<SWidget> GetAccessibleWidget() const override;
#endif
};
