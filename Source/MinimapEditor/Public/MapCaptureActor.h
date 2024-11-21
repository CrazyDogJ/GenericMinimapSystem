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

UCLASS()
class MINIMAPEDITOR_API AMapCaptureActor : public AActor
{
	GENERATED_BODY()

private:
	UPROPERTY()
	USceneComponent* SceneComponent;
	
	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget;
public:	
	// Sets default values for this actor's properties
	AMapCaptureActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UFUNCTION(CallInEditor)
	void CaptureMap();

	void SaveMapInfo(UMinimapMapData* NewDataAsset, const FString& Path, const FString& Name);

	void WriteMapInfo(UMinimapMapData* DataAsset, UTexture2D* Tex);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneCaptureComponent2D* Capture2D;
	
	UPROPERTY(EditAnywhere)
	int32 TextureScale = 2048;

	UPROPERTY(EditAnywhere, meta = (MakeEditWidget))
	FVector EndPoint = FVector(5,5,0);

protected:

	virtual void OnConstruction(const FTransform& Transform) override;

};
