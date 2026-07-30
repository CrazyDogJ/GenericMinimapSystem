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

USTRUCT(BlueprintType)
struct MINIMAPEDITOR_API FMapCaptureSequence
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString SpecificFolder = "";
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 LodIndex = 0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 TileAxisCount = 1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 ResolutionPower = 10;
};

struct MINIMAPEDITOR_API FTileCaptureResult
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
	
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(CallInEditor)
	void CaptureMap();
	UFUNCTION(CallInEditor)
	void CaptureHotPoints() const;
	UFUNCTION(CallInEditor)
	void CaptureSingleMapTexture();
	UFUNCTION(CallInEditor)
	void CaptureMapSequence();

	void WriteMapInfo(UMinimapMapData* DataAsset, UTexture2D* Tex);
	void WriteHotPoints(UMinimapMapData* DataAsset) const;
	
	UPROPERTY(EditAnywhere)
	TArray<FMapCaptureSequence> Sequences;
	
	UPROPERTY(EditAnywhere)
	USceneCaptureComponent2D* Capture2D;

	TArray<FTileCaptureResult> AllTiles;
	int CurrentTileIndex = 0;
	TMap<FIntPoint, FVector> TilePositions;
	void StartCapture();
	void CaptureNextTile();

	UPROPERTY(EditAnywhere)
	FIntPoint SingleCapture2D;
	
	UPROPERTY(EditAnywhere)
	int32 LodCount = 1;
	
	UPROPERTY(EditAnywhere)
	int32 TileAxisCount = 4;
	
	UPROPERTY(EditAnywhere)
	int32 Power = 10;
	
	UPROPERTY(VisibleAnywhere)
	int32 TextureScale = 1024;

	UPROPERTY(EditAnywhere, meta = (MakeEditWidget))
	FVector EndPoint = FVector(5,5,0);

	UPROPERTY(EditAnywhere)
	bool bLocalMap = false;

	UPROPERTY(EditAnywhere)
	FString SpecificFolder = "";
	
	UPROPERTY(EditAnywhere, meta = (EditCondition=bLocalMap))
	FString MapName;

protected:
	static void OverwriteExistingTextureFromRenderTarget(UTexture2D* TargetTexture, UTextureRenderTarget2D* SourceRT);
	static void CaptureMiniMapToPath(const FString& PackagePath, UTextureRenderTarget2D* SourceRT);
};
