#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "NetTypeSplineComponent.generated.h"

class UWaterSplineMetadata;
class UNetTypeSplineComponent;

USTRUCT(BlueprintType)
struct FNetTypeSplineConnection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UNetTypeSplineComponent* SplineComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bStartOrEnd = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ConnectionDistance = 0.0f;

	bool operator==(const FNetTypeSplineConnection& other) const
	{
		return other.SplineComponent == SplineComponent;
	}
};

UCLASS()
class UNetTypeSplineMetaData : public USplineMetadata
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Net Type")
	FInterpCurveFloat Width;

	UPROPERTY(EditAnywhere, Category="Net Type")
	FInterpCurveFloat Height;

	/** Insert point before index, lerping metadata between previous and next key values */
	virtual void InsertPoint(int32 Index, float t, bool bClosedLoop) override;
	/** Update point at index by lerping metadata between previous and next key values */
	virtual void UpdatePoint(int32 Index, float t, bool bClosedLoop) override;
	virtual void AddPoint(float InputKey) override;
	virtual void RemovePoint(int32 Index) override;
	virtual void DuplicatePoint(int32 Index) override;
	virtual void CopyPoint(const USplineMetadata* FromSplineMetadata, int32 FromIndex, int32 ToIndex) override;
	virtual void Reset(int32 NumPoints) override;
	virtual void Fixup(int32 NumPoints, USplineComponent* SplineComp) override;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MINIMAP_API UNetTypeSplineComponent : public USplineComponent
{
	GENERATED_BODY()

public:
	UNetTypeSplineComponent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spline")
	TArray<FNetTypeSplineConnection> Connections;
	
	virtual USplineMetadata* GetSplinePointsMetadata() override;
	virtual const USplineMetadata* GetSplinePointsMetadata() const override;
};
