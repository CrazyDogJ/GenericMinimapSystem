// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapRecastNavMesh.h"


// Sets default values
AMinimapRecastNavMesh::AMinimapRecastNavMesh()
{
	SetCellSize(ENavigationDataResolution::Low, 100.0f);
	TileSizeUU = 5000;
	AgentRadius = 50.0f;
}

// Called when the game starts or when spawned
void AMinimapRecastNavMesh::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMinimapRecastNavMesh::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

