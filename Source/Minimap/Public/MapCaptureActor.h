// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapCaptureActor.generated.h"

class USceneCaptureComponent2D;
class USceneComponent;
class UTextureRenderTarget2D;
class UMinimapSettings;

UCLASS()
class MINIMAP_API AMapCaptureActor : public AActor
{
	GENERATED_BODY()

private:
	UPROPERTY()
	USceneCaptureComponent2D* Capture2D;
	
	UPROPERTY()
	USceneComponent* SceneComponent;
	
	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget;

	UFUNCTION()
	static int32 HasConfig(UMinimapSettings* Settings, const FString& LevelName);
public:	
	// Sets default values for this actor's properties
	AMapCaptureActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UFUNCTION(CallInEditor)
	void CaptureMap();

	UPROPERTY(EditAnywhere)
	int32 TextureScale = 2048;

	UPROPERTY(EditAnywhere, meta = (MakeEditWidget))
	FVector EndPoint = FVector(5,5,0);

protected:

	virtual void OnConstruction(const FTransform& Transform) override;

};
