#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetTypeSplineActor.generated.h"

class UNetTypeSplineMetaData;
class UNetTypeSplineComponent;

UCLASS(Placeable)
class MINIMAP_API ANetTypeSplineActor : public AActor
{
	GENERATED_BODY()

public:
	ANetTypeSplineActor(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(Instanced)
	TObjectPtr<UNetTypeSplineMetaData> NetTypeSplineMetadata;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<UNetTypeSplineComponent*> SplineComponents;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UNetTypeSplineComponent* RootSpline;

	void RemoveSplineComponent(UNetTypeSplineComponent* EditComp);
	void NewConnectionAtDistance(UNetTypeSplineComponent* EditComp, float Distance);

	virtual void OnConstruction(const FTransform& Transform) override;
};
