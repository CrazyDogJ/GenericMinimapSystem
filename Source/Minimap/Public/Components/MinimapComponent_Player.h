// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "MinimapComponent.h"
#include "MinimapFastArray.h"
#include "MinimapComponent_Player.generated.h"

class UMainMapUserWidget;
class UMinimapUserWidget;

/** Minimap save data struct for player component. */
USTRUCT(BlueprintType)
struct FMinimapSaveData
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame, BlueprintReadOnly)
	bool bHasTempPin = false;
	
	UPROPERTY(SaveGame, BlueprintReadOnly)
	FVector TempPinLocation = FVector::ZeroVector;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TMap<FString, FMinimapIndices> HotPointSaveGames;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShownMapPinEvent, FGuid, Guid);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocalMinimapChanged, const UMinimapMapData*, ChangedMinimapData);

/**
 * 
 */
UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class MINIMAP_API UMinimapComponent_Player : public UMinimapComponent
{
	GENERATED_BODY()
	
public:
	UMinimapComponent_Player(const FObjectInitializer& ObjectInitializer);
	
protected:
	// If you enter a local minimap area, this will be changed.
	UPROPERTY(BlueprintReadOnly)
	UMinimapMapData* CurrentLocalMinimapData = nullptr;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(Client, Reliable)
	void ReceiveControllerChangedDelegate(APawn* Pawn, AController* OldController, AController* NewController);
	void ControllerChanged(const AController* NewController);
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostLoad() override;
	virtual void NativeGetDisplayNameAndDescription(FText& DisplayName, FText& Description) override;
	
public:
	// Properties
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite)
	FSlateBrush TempPinBrush;

	UPROPERTY(BlueprintReadOnly)
	APawn* OwnerPawn;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Replicated)
	FPoiStateList PoiStateList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSet<FGameplayTag> HiddenCategoryTags;
	
	UPROPERTY(BlueprintReadWrite, Replicated)
	AMapPinActor* TempPin;

	UPROPERTY(BlueprintAssignable)
	FOnLocalMinimapChanged OnLocalMinimapChanged;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UMinimapUserWidget* MinimapUserWidget = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UMainMapUserWidget* MainMapUserWidget = nullptr;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TSubclassOf<UMinimapUserWidget> MinimapUserWidgetClass;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TSubclassOf<UMainMapUserWidget> MainMapUserWidgetClass;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TSoftObjectPtr<UInputMappingContext> InputMappingContext = nullptr;
	
	// Functions
	UMinimapMapData* GetCurrentLocalMinimapData() const {return CurrentLocalMinimapData;}
	void SetCurrentLocalMinimapData(UMinimapMapData* MinimapData);
	
	UFUNCTION(BlueprintCallable)
	UMainMapUserWidget* GetOrCreateMainMapWidget();

	UFUNCTION(BlueprintImplementableEvent)
	void CreateAdditionalWidgets();

	UFUNCTION(BlueprintImplementableEvent)
	void RemoveAdditionalWidgets();
	
	/**
	 * Add temp pin at the mid of screen.
	 */
	UFUNCTION(BlueprintCallable)
	void AddTempPin();

	UFUNCTION(BlueprintCallable)
	void AddTempPin_MainMap(const FVector2D Location, const UMinimapMapData* MapData, const ECollisionChannel TraceChannel);

	UFUNCTION(BlueprintCallable)
	void RemoveTempPin_MainMap();

	/** TODO : Unique color is not work when subsystem is LocalPlayerSubsystem
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void SetUniqueColorIndex();
	*/
	
	void AddTempPinImplement(const FVector& Location);
	
	UFUNCTION(Server, Reliable)
	void AddTempPinExec(FVector Location);

	// Fix on listen server
	UFUNCTION(NetMulticast, Reliable)
	void AddTempPinMulticast(FVector Location);

	UFUNCTION(Server, Reliable)
	void RemoveTempPinExec();

	UFUNCTION(BlueprintCallable)
	FMinimapSaveData GetSaveData() const;
	
	UFUNCTION(BlueprintCallable)
	void LoadSaveData(FMinimapSaveData inData, UTexture2D* MapMaskData);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool HotPointCheck(FGuid Guid) const;

	UFUNCTION(BlueprintCallable)
	TMap<FString, FMinimapIndices> GetFoundHotPoints() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsHotPointFound(const FString& LevelName, const FGuid& PoiIndex) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void FindHotPoint(const FString& LevelName, const FGuid& PoiIndex, bool Global);
	
	bool GetHitResultAtScreenPosition(const FVector2D ScreenPosition, const ECollisionChannel TraceChannel, const FCollisionQueryParams& CollisionQueryParams, FHitResult& HitResult) const;

#pragma region Render Target
private:
	UPROPERTY(EditDefaultsOnly)
	bool bCreateRenderTarget = false;

	void CreateRenderTarget();
public:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UTextureRenderTarget2D* RT;

	UPROPERTY(EditDefaultsOnly)
	int Resolution = 1024;

	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* MaskLoadMaterial;

	UPROPERTY(EditDefaultsOnly)
	FName TexturePropertyName = FName("RT");
	
	UFUNCTION(BlueprintPure)
	bool GetShouldCreateRenderTarget() const { return bCreateRenderTarget; }

	UFUNCTION(BlueprintCallable)
	void SetRenderTarget(bool bInRenderTarget = true);

	TArray<uint8> SerializeRenderTargetData (int32& Width, int32& Height) const;

	UFUNCTION(BlueprintCallable)
	void SendRenderTargetData();
	
	UFUNCTION(Server, Reliable)
	void SendRenderTargetToServer(const int32& Width, const int32& Height, const TArray<uint8>& Data);

	UFUNCTION(NetMulticast, Reliable)
	void SendRenderTargetToClients(const int32& Width, const int32& Height, const TArray<uint8>& Data);

	UFUNCTION(BlueprintImplementableEvent)
	void OnRenderTargetReceived(UTexture2D* Texture2D);
#pragma endregion Render Target

#pragma region Nav Query
public:
	// Nav query for background query.
	/** Should auto update nav query start location. */
	UPROPERTY(BlueprintReadWrite, Category = "Nav Query")
	bool bAutoUpdateStartLocation = true;

	/** Should do nav query update. */
	UPROPERTY(BlueprintReadWrite, Category = "Nav Query")
	bool bShouldUpdateNavQuery = false;

	UPROPERTY(BlueprintReadOnly, Category = "Nav Query")
	bool bPathPointsValid = false;
	
	UPROPERTY(BlueprintReadWrite, Category = "Nav Query")
	FVector NavQueryStartPosition;

	UPROPERTY(BlueprintReadWrite, Category = "Nav Query")
	FVector NavQueryEndPosition;

	UPROPERTY(BlueprintReadWrite, Category = "Nav Query")
	FVector NavQueryExtend = FVector(10000.0f);
	
	UPROPERTY(BlueprintReadOnly, Category = "Nav Query")
	TArray<FVector> NavQueryOutPathPoints;

	float NavQueryTime = 0.0f;
	
	UPROPERTY(BlueprintReadWrite, Category = "Nav Query")
	float NavQueryPeriod = 1.0f;

	UFUNCTION(BlueprintPure, Category = "Nav Query")
	bool ShouldShowNavPath() const;

	void UpdateNavPath(const float& DeltaTime);
#pragma endregion Nav Query

#pragma region Minimap Widget
public:
	UPROPERTY(BlueprintReadOnly, meta=(Units = "cm"))
	float MinimapRadius = 10000.0f;

	UPROPERTY(BlueprintAssignable)
	FShownMapPinEvent OnMapPinShowOnMinimap;

	UPROPERTY(BlueprintAssignable)
	FShownMapPinEvent OnMapPinHideOnMinimap;

	UFUNCTION(BlueprintCallable)
	void SetMinimapRadius(float Radius);

	UFUNCTION(BlueprintPure)
	FMapPinBase GetShownMinimapPin(FGuid Guid, bool& Success) const;
	
	//Helper functions
	TArray<FGuid> GetShownMapPins() const { return ShownMapPinsGuids; }
	void AddMinimapPin(FGuid Guid);
	void RemoveMinimapPin(FGuid Guid);
	
protected:
	/* All map pins shows on minimap */
	TArray<FGuid> ShownMapPinsGuids;

	UFUNCTION()
	void OnStaticRegistered(const FStaticMapPin& StaticMapPin);

	UFUNCTION()
	void OnStaticUnregistered(const FStaticMapPin& StaticMapPin);
	
	UFUNCTION()
	void OnComponentUnregistered(UMinimapComponent* Component);
    	
	void UpdateMinimapShownPins();
	
#pragma endregion Minimap Widget
};
