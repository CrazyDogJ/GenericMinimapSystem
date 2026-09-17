// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapComponent.h"
#include "MinimapComponent_Local.generated.h"

UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class MINIMAP_API UMinimapComponent_Local : public UMinimapComponent
{
	GENERATED_BODY()

public:
	UMinimapComponent_Local();
};
