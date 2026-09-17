
#include "Actors/MinimapLocalAreaActor.h"

#include "MinimapSubsystem.h"
#include "Components/BrushComponent.h"
#include "Components/CapsuleComponent.h"

AMinimapLocalAreaActor::AMinimapLocalAreaActor()
{
	PrimaryActorTick.bCanEverTick = false;
	GetBrushComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	GetBrushComponent()->OnComponentBeginOverlap.AddDynamic(this, &AMinimapLocalAreaActor::OnBrushOverlapped);
	GetBrushComponent()->OnComponentEndOverlap.AddDynamic(this, &AMinimapLocalAreaActor::OnBrushEndOverlap);
}

void AMinimapLocalAreaActor::OnBrushOverlapped(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	const auto ShapeComponent = Cast<UShapeComponent>(OtherComp);
	const auto IsRoot = ShapeComponent->GetOwner()->GetRootComponent() == ShapeComponent;
	const auto IsPawn = Cast<APawn>(ShapeComponent->GetOwner());
	if (IsPawn && IsRoot && IsPawn->IsLocallyControlled())
	{
		if (const auto Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>())
		{
			Sub->SetLocalMinimapMapData(LocalMinimapData);
		}
	}
}

void AMinimapLocalAreaActor::OnBrushEndOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor,
	class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	const auto ShapeComponent = Cast<UShapeComponent>(OtherComp);
	const auto IsRoot = ShapeComponent->GetOwner()->GetRootComponent() == ShapeComponent;
	const auto IsPawn = Cast<APawn>(ShapeComponent->GetOwner());
	if (IsPawn && IsRoot && IsPawn->IsLocallyControlled())
	{
		if (const auto Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>())
		{
			if (Sub->LocalMinimapMapData == LocalMinimapData)
			{
				Sub->SetLocalMinimapMapData(nullptr);
			}
		}
	}
}
