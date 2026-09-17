// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MapPinUserWidget.generated.h"

class IMinimapWidgetInterface;

/** Base class of map pins. */
UCLASS()
class MINIMAP_API UMapPinUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Interface pointer to get data.
	UPROPERTY()
	TWeakObjectPtr<UObject> MinimapDataSourceObject;
	
	UFUNCTION(BlueprintPure)
	FSlateBrush GetMapPinBrush();
	
	IMinimapWidgetInterface* TryGetInterface() const;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap", meta=(ExposeOnSpawn))
	FGuid Guid;
};
