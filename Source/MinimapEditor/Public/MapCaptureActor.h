// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapMapData.h"
#include "GameFramework/Actor.h"
#include "MapCaptureActor.generated.h"

class USceneCaptureComponent2D;
class USceneComponent;
class UTextureRenderTarget2D;
class UMinimapSettings;

struct FTileCaptureResult
{
	int32 TileX;
	int32 TileY;
	int32 Size;
	TArray<FColor> Pixels;
};

UCLASS()
class MINIMAPEDITOR_API AMapCaptureActor : public AActor
{
	GENERATED_BODY()

private:
	UPROPERTY()
	USceneComponent* SceneComponent;
	
public:	
	// Sets default values for this actor's properties
	AMapCaptureActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UFUNCTION(CallInEditor)
	void CaptureMap();

	void SaveMapInfo(UMinimapMapData* NewDataAsset, const FString& Path, const FString& Name);

	void WriteMapInfo(UMinimapMapData* DataAsset, UTexture2D* Tex);
	
	UPROPERTY(EditAnywhere)
	USceneCaptureComponent2D* Capture2D;

	TArray<FTileCaptureResult> AllTiles;
	int CurrentTileIndex = 0;
	TMap<FIntPoint, FVector> TilePositions;
	void StartCapture();
	void CaptureNextTile();
	void CaptureFinished();
	
	UPROPERTY(EditAnywhere)
	int32 TilePower = 10;

	UPROPERTY(VisibleAnywhere)
	int32 TileSize = 1024;
	
	UPROPERTY(EditAnywhere)
	int32 Power = 12;
	
	UPROPERTY(VisibleAnywhere)
	int32 TextureScale = 4096;

	UPROPERTY(EditAnywhere, meta = (MakeEditWidget))
	FVector EndPoint = FVector(5,5,0);

	UPROPERTY(EditAnywhere)
	bool bLocalMap = false;

	UPROPERTY(EditAnywhere, meta = (EditCondition=bLocalMap))
	FString MapName;

protected:

	virtual void OnConstruction(const FTransform& Transform) override;

};
