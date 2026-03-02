// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldPartition/HLOD/HLODActor.h"
#include "MinimapWPHLOD.generated.h"

UCLASS()
class MINIMAP_API AMinimapWPHLOD : public AWorldPartitionHLOD
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMinimapWPHLOD();

	virtual void PreRegisterAllComponents() override;

#if WITH_EDITOR
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif
	// Update collision avoid something world.
	virtual void SetVisibility(bool bIsVisible) override;
};
