// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapPinActor.h"
#include "Blueprint/UserWidget.h"
#include "MapPinWidget.generated.h"

/**
 * 
 */
UCLASS()
class MINIMAP_API UMapPinWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AMapPinActor* OwnerActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSlateBrush Brush;
};
