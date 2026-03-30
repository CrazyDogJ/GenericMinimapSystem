// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapStructs.h"
#include "Blueprint/UserWidget.h"
#include "MapPinUserWidget.generated.h"

UCLASS()
class MINIMAP_API UMapPinUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap", meta=(ExposeOnSpawn))
	FGuid Guid;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Minimap", meta=(ExposeOnSpawn))
	FMapPinBase MapPinInfo;
};
