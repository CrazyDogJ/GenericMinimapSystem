// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "MinimapGlobal.h"
#include "NetRelevantLocalComponent.h"
#include "MinimapPlayerComponent.generated.h"

class UMainMapUserWidget;
class UMinimapUserWidget;

// Put this on player controller.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MINIMAP_API UMinimapPlayerComponent : public UNetRelevantLocalComponent
{
	GENERATED_BODY()

public:
	UMinimapPlayerComponent();
	
public:
	UFUNCTION()
	void OnMinimapGlobalReadyEvent(UMinimapGlobal* MinimapGlobal);
	
	virtual void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UMinimapUserWidget* MinimapUserWidget = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UMainMapUserWidget* MainMapUserWidget = nullptr;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TSoftObjectPtr<UInputMappingContext> InputMappingContext = nullptr;
	
	UFUNCTION(BlueprintCallable)
	UMinimapUserWidget* GetOrCreateMinimapWidget(UObject* DataSourceObject);
	
	UFUNCTION(BlueprintCallable)
	UMainMapUserWidget* GetOrCreateMainMapWidget(UObject* DataSourceObject);
	
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
};
