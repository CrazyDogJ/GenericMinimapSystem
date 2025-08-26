// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NavMesh/RecastNavMesh.h"
#include "MinimapRecastNavMesh.generated.h"

UCLASS()
class MINIMAP_API AMinimapRecastNavMesh : public ARecastNavMesh
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMinimapRecastNavMesh();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
