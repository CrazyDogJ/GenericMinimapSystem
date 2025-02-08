
#pragma once

#include "CoreMinimal.h"
#include "MinimapMapData.h"
#include "GameFramework/Volume.h"
#include "MinimapLocalAreaActor.generated.h"

UCLASS()
class MINIMAP_API AMinimapLocalAreaActor : public AVolume
{
	GENERATED_BODY()

public:
	AMinimapLocalAreaActor();

	UFUNCTION()
	void OnBrushOverlapped(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnBrushEndOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UMinimapMapData* LocalMinimapData;

};
