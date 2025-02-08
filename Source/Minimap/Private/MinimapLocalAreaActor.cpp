
#include "MinimapLocalAreaActor.h"

#include "MinimapComponent_Player.h"
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
	if (Cast<UCapsuleComponent>(OtherComp))
	{
		if (auto MinimapPlayerComp = Cast<UMinimapComponent_Player>(OtherComp->GetOwner()->GetComponentByClass(UMinimapComponent_Player::StaticClass())))
		{
			MinimapPlayerComp->SetCurrentLocalMinimapData(LocalMinimapData);
		}
	}
}

void AMinimapLocalAreaActor::OnBrushEndOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor,
	class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Cast<UCapsuleComponent>(OtherComp))
	{
		if (auto MinimapPlayerComp = Cast<UMinimapComponent_Player>(OtherComp->GetOwner()->GetComponentByClass(UMinimapComponent_Player::StaticClass())))
		{
			if (MinimapPlayerComp->GetCurrentLocalMinimapData() == LocalMinimapData)
			{
				MinimapPlayerComp->SetCurrentLocalMinimapData(nullptr);
			}
		}
	}
}
