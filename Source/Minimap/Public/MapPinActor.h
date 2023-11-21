// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapComponent.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "MapPinActor.generated.h"

UCLASS()
class MINIMAP_API AMapPinActor : public AActor
{
	GENERATED_BODY()

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:	
	// Sets default values for this actor's properties
	AMapPinActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY()
	USphereComponent* SphereCollision;

	UPROPERTY()
	USceneComponent* SceneComponent;
	
	UPROPERTY()
	UMinimapComponent* MinimapComp;

	UPROPERTY()
	UWidgetComponent* WidgetComponent;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite)
	FSlateBrush PinSlateBrush;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition = "bCollision"))
	float CollisionRadius = 64.f;

	UFUNCTION()
	void Overlapped(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,int32 OtherBodyIndex,bool bFromSweep,const FHitResult& SweepResult);
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called when the game starts or when spawned
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void OnConstruction(const FTransform& Transform) override;
};
