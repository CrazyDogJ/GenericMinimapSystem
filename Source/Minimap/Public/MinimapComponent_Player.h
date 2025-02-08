// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinimapComponent.h"
#include "MinimapComponent_Player.generated.h"

USTRUCT(BlueprintType)
struct FHotPointSaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, SaveGame)
	FString LevelName;

	UPROPERTY(SaveGame)
	TMap<FGuid, bool> HotPointFoundMap;
};

USTRUCT(BlueprintType)
struct FMinimapSaveData
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame, BlueprintReadOnly)
	bool bHasTempPin = false;
	
	UPROPERTY(SaveGame, BlueprintReadOnly)
	FVector TempPinLocation;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<FHotPointSaveGame> HotPointSaveGames;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocalMinimapChanged, UMinimapMapData*, ChangedMinimapData);

/**
 * 
 */
UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class MINIMAP_API UMinimapComponent_Player : public UMinimapComponent
{
	GENERATED_BODY()

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UMinimapComponent_Player(const FObjectInitializer& ObjectInitializer);
	
public:
	// Properties
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite)
	FSlateBrush TempPinBrush;

	UPROPERTY(BlueprintReadOnly)
	APawn* OwnerPawn;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UTextureRenderTarget2D* RT;

	UPROPERTY(EditDefaultsOnly)
	int Resolution = 1024;

	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* MaskLoadMaterial;

	UPROPERTY(EditDefaultsOnly)
	FName TexturePropertyName = FName("RT");

	UPROPERTY(BlueprintReadOnly)
	TArray<FHotPointSaveGame> HotPointSaveGames;

protected:
	// If you enter a local minimap area, this will be changed.
	UPROPERTY(BlueprintReadOnly)
	UMinimapMapData* CurrentLocalMinimapData = nullptr;

	// Functions
public:
	UPROPERTY(BlueprintAssignable)
	FOnLocalMinimapChanged OnLocalMinimapChanged;
	
	UMinimapMapData* GetCurrentLocalMinimapData() const {return CurrentLocalMinimapData;}
	void SetCurrentLocalMinimapData(UMinimapMapData* MinimapData);
	
	UFUNCTION(BlueprintPure)
	bool ShouldVisible() const;
	
	/**
	 * Add temp pin at the mid of screen.
	 */
	UFUNCTION(BlueprintCallable)
	void AddTempPin();

	UFUNCTION(BlueprintCallable)
	void AddTempPin_MainMap(const FVector2D Location, const UMinimapMapData* MapData, const ECollisionChannel TraceChannel);

	UFUNCTION(BlueprintCallable)
	void RemoveTempPin_MainMap();
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void SetUniqueColorIndex();

	UFUNCTION(Server, Reliable)
	void AddTempPinExec(FVector Location);

	UFUNCTION(Server, Reliable)
	void RemoveTempPinExec();

	UFUNCTION(BlueprintCallable)
	FMinimapSaveData GetSaveData();
	
	UFUNCTION(BlueprintCallable)
	void LoadSaveData(FMinimapSaveData inData, UTexture2D* MapMaskData);

	UFUNCTION(BlueprintCallable)
	bool IsHotPointFound(FHotPointInfo HotPointInfo);
	
	bool GetHitResultAtScreenPosition(const FVector2D ScreenPosition, const ECollisionChannel TraceChannel, const FCollisionQueryParams& CollisionQueryParams, FHitResult& HitResult) const;

	TArray<uint8> SerializeRenderTargetData (int32& Width, int32& Height) const;

	UFUNCTION(BlueprintCallable)
	void SendRenderTargetData();
	
	UFUNCTION(Server, Reliable)
	void SendRenderTargetToServer(const int32& Width, const int32& Height, const TArray<uint8>& Data);

	UFUNCTION(NetMulticast, Reliable)
	void SendRenderTargetToClients(const int32& Width, const int32& Height, const TArray<uint8>& Data);

	UFUNCTION(BlueprintImplementableEvent)
	void OnRenderTargetReceived(UTexture2D* Texture2D);
protected:
	virtual void BeginPlay() override;

	virtual void PostLoad() override;
};
