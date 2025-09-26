#include "NetTypeSplineActor.h"

#include "NetTypeSplineComponent.h"

ANetTypeSplineActor::ANetTypeSplineActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootSpline = CreateDefaultSubobject<UNetTypeSplineComponent>(TEXT("RootSpline"));
	SplineComponents.Add(RootSpline);
	
	NetTypeSplineMetadata = ObjectInitializer.CreateDefaultSubobject<UNetTypeSplineMetaData>(this, TEXT("NetTypeSplineMetadata"));
	NetTypeSplineMetadata->Reset(2);
	NetTypeSplineMetadata->AddPoint(0.0f);
	NetTypeSplineMetadata->AddPoint(1.0f);
}

void ANetTypeSplineActor::RemoveSplineComponent(UNetTypeSplineComponent* EditComp)
{
	if (EditComp)
	{
		Modify();
		
		MarkPackageDirty();
	}
}

void ANetTypeSplineActor::NewConnectionAtDistance(UNetTypeSplineComponent* EditComp, float Distance)
{
	if (EditComp)
	{
		Modify();

		EditComp->Connections.Add(FNetTypeSplineConnection(nullptr, true, Distance));
		
		MarkPackageDirty();
	}
}

void ANetTypeSplineActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	for (auto& SplineComponent : SplineComponents)
	{
		if (SplineComponent)
		{
			for (auto& Connection : SplineComponent->Connections)
			{
				const auto NetTypeSpline = Cast<UNetTypeSplineComponent>(
					AddComponentByClass(UNetTypeSplineComponent::StaticClass(), false, FTransform::Identity, false));

				Connection.SplineComponent = NetTypeSpline;
				// Use distance to update location.
				NetTypeSpline->SetRelativeLocation(SplineComponent->GetLocationAtDistanceAlongSpline(Connection.ConnectionDistance, ESplineCoordinateSpace::Local));
			}
		}
	}
}
