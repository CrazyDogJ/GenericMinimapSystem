// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#if UE_WITH_IRIS && ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
#include "Iris/ReplicationSystem/NetObjectGroupHandle.h"
#include "Net/Core/NetBitArray.h"
#endif
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
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_OwningController)
	AController* OwningController;

	UFUNCTION()
	void OnRep_OwningController();
	
	FGuid Id;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSlateBrush PinSlateBrush;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (EditCondition = "bCollision"))
	float CollisionRadius = 64.f;

	UFUNCTION()
	void Overlapped(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,int32 OtherBodyIndex,bool bFromSweep,const FHitResult& SweepResult);
	
protected:
	UFUNCTION()
	void OnControllerDestroyed(AActor* DestroyedActor);
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
	
#if UE_WITH_IRIS && ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
	virtual void OnReplicationStartedForIris(const FOnReplicationStartedParams& Params) override;
	virtual void OnStopReplicationForIris(const FOnStopReplicationParams& Params) override;
	void UpdateIrisViewers(float DeltaSeconds);
	void ReleaseIrisViewerGroup();
	TWeakObjectPtr<UReplicationSystem> IrisSystem;
	UE::Net::FNetObjectGroupHandle IrisViewerGroup;
	UE::Net::FNetBitArray IrisAllowedConnections;
	UE::Net::FNetBitArray IrisNextConnections;
	FDelegateHandle IrisFlushHandle;
#endif

	void AddMapPin();
	void InitializeOwningController();
	void InitializeCollision();
	
	// Called when the game starts or when spawned
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


};
