#include "NetTypeSplineComponent.h"

#include "NetTypeSplineActor.h"

void UNetTypeSplineMetaData::InsertPoint(int32 Index, float t, bool bClosedLoop)
{
	check(Index >= 0);

	Modify();

	int32 NumPoints = Width.Points.Num();
	float InputKey = static_cast<float>(Index);

	if(Index >= NumPoints)
	{ 
		// Just add point to the end instead of trying to insert
		AddPoint(InputKey);
	}
	else
	{
		int32 PrevIndex = (bClosedLoop && Index == 0 ? NumPoints - 1 : Index - 1);
		bool bHasPrevIndex = (PrevIndex >= 0 && PrevIndex < NumPoints);

		float NewWidthVal = Width.Points[Index].OutVal;
		float NewHeightVal = Height.Points[Index].OutVal;

		if (bHasPrevIndex)
		{
			float PrevHeightVal = Height.Points[PrevIndex].OutVal;
			float PrevWidthVal = Width.Points[PrevIndex].OutVal;
			NewHeightVal = FMath::LerpStable(PrevHeightVal, NewHeightVal, t);
			NewWidthVal = FMath::LerpStable(PrevWidthVal, NewWidthVal, t);
		}

		FInterpCurvePoint<float> NewDepth(InputKey, NewWidthVal);
		Width.Points.Insert(NewDepth, Index);

		FInterpCurvePoint<float> NewLevel(InputKey, NewHeightVal);
		Height.Points.Insert(NewLevel, Index);

		for (int32 i = Index + 1; i < Width.Points.Num(); ++i)
		{
			Width.Points[i].InVal += 1.0f;
			Height.Points[i].InVal += 1.0f;
		}
	}
}

void UNetTypeSplineMetaData::UpdatePoint(int32 Index, float t, bool bClosedLoop)
{
	const int32 NumPoints = Width.Points.Num();
	check(Index >= 0 && Index < NumPoints);

	int32 PrevIndex = (bClosedLoop && Index == 0 ? NumPoints - 1 : Index - 1);
	int32 NextIndex = (bClosedLoop && Index + 1 > NumPoints ? 0 : Index + 1);
	
	bool bHasPrevIndex = (PrevIndex >= 0 && PrevIndex < NumPoints);
	bool bHasNextIndex = (NextIndex >= 0 && NextIndex < NumPoints);

	Modify();

	if (bHasPrevIndex && bHasNextIndex)
	{
		float PrevHeightVal = Height.Points[PrevIndex].OutVal;
		float PrevWidthVal = Width.Points[PrevIndex].OutVal;

		float NextHeightVal = Height.Points[NextIndex].OutVal;
		float NextWidthVal = Width.Points[NextIndex].OutVal;

		Height.Points[Index].OutVal = FMath::LerpStable(PrevHeightVal, NextHeightVal, t);
		Width.Points[Index].OutVal = FMath::LerpStable(PrevWidthVal, NextWidthVal, t);
	}
}

void UNetTypeSplineMetaData::AddPoint(float InputKey)
{
	Modify();
	
	float NewHeightVal = 0;
	float NewWidthVal = 0;
	
	int Index = Width.Points.Num() - 1;

	if (Index >= 0)
	{
		NewHeightVal = Height.Points[Index].OutVal;
		NewWidthVal = Width.Points[Index].OutVal;
	}

	float NewInputKey = static_cast<float>(++Index);
	Height.Points.Emplace(NewInputKey, NewHeightVal);
	Width.Points.Emplace(NewInputKey, NewWidthVal);
}

void UNetTypeSplineMetaData::RemovePoint(int32 Index)
{
	check(Index < Width.Points.Num());

	Modify();
	Height.Points.RemoveAt(Index);
	Width.Points.RemoveAt(Index);

	for (int32 i = Index; i < Width.Points.Num(); ++i)
	{
		Height.Points[i].InVal -= 1.0f;
		Width.Points[i].InVal -= 1.0f;
	}
}

void UNetTypeSplineMetaData::DuplicatePoint(int32 Index)
{
	check(Index < Width.Points.Num());

	Modify();
	
	Height.Points.Insert(FInterpCurvePoint<float>(Height.Points[Index]), Index);
	Width.Points.Insert(FInterpCurvePoint<float>(Width.Points[Index]), Index);

	for (int32 i = Index + 1; i < Width.Points.Num(); ++i)
	{
		Height.Points[i].InVal += 1.0f;
		Width.Points[i].InVal += 1.0f;
	}
}

void UNetTypeSplineMetaData::CopyPoint(const USplineMetadata* FromSplineMetadata, int32 FromIndex, int32 ToIndex)
{
	check(FromSplineMetadata != nullptr);

	if (const UNetTypeSplineMetaData* FromMetadata = Cast<UNetTypeSplineMetaData>(FromSplineMetadata))
	{
		check(ToIndex < Width.Points.Num());
		check(FromIndex < FromMetadata->Width.Points.Num());

		Modify();
		Height.Points[ToIndex].OutVal = FromMetadata->Height.Points[FromIndex].OutVal;
		Width.Points[ToIndex].OutVal = FromMetadata->Width.Points[FromIndex].OutVal;
	}
}

void UNetTypeSplineMetaData::Reset(int32 NumPoints)
{
	Modify();
	Height.Points.Reset(NumPoints);
	Width.Points.Reset(NumPoints);
}

#if WITH_EDITORONLY_DATA
template <class T>
void FixupCurve(FInterpCurve<T>& Curve, const T& DefaultValue, int32 NumPoints)
{
	// Fixup bad InVal values from when the add operation below used the wrong value
	for (int32 PointIndex = 0; PointIndex < Curve.Points.Num(); PointIndex++)
	{
		float InVal = PointIndex;
		Curve.Points[PointIndex].InVal = InVal;
	}

	while (Curve.Points.Num() < NumPoints)
	{
		// InVal is the point index which is ascending so use previous point plus one.
		float InVal = Curve.Points.Num() > 0 ? Curve.Points[Curve.Points.Num() - 1].InVal + 1.0f : 0.0f;
		Curve.Points.Add(FInterpCurvePoint<T>(InVal, DefaultValue));
	}
	
	if (Curve.Points.Num() > NumPoints)
	{
		Curve.Points.RemoveAt(NumPoints, Curve.Points.Num()-NumPoints);
	}
}
#endif

void UNetTypeSplineMetaData::Fixup(int32 NumPoints, USplineComponent* SplineComp)
{
#if WITH_EDITORONLY_DATA
	FixupCurve(Height, 0.0f, NumPoints);
	FixupCurve(Width, 0.0f, NumPoints);
#endif
}

UNetTypeSplineComponent::UNetTypeSplineComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bSplineHasBeenEdited = true;
}

USplineMetadata* UNetTypeSplineComponent::GetSplinePointsMetadata()
{
	if (const auto OwnerSplineActor = Cast<ANetTypeSplineActor>(GetOwner()))
	{
		return OwnerSplineActor->NetTypeSplineMetadata;
	}
	
	return nullptr;
}

const USplineMetadata* UNetTypeSplineComponent::GetSplinePointsMetadata() const
{
	if (const auto OwnerSplineActor = Cast<ANetTypeSplineActor>(GetOwner()))
    {
    	return OwnerSplineActor->NetTypeSplineMetadata;
    }
    
    return nullptr;
}
