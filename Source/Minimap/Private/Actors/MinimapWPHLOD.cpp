// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/MinimapWPHLOD.h"
#include "PhysicsEngine/BodySetup.h"

AMinimapWPHLOD::AMinimapWPHLOD()
{
}

void AMinimapWPHLOD::PreRegisterAllComponents()
{
	Super::PreRegisterAllComponents();

#if WITH_EDITOR
	if (GetWorld() && !IsRunningCommandlet() && !FApp::IsUnattended())
	{
		// Epic Games! We need query collision!
		SetActorEnableCollision(true);
		ForEachComponent<UPrimitiveComponent>(false, [](UPrimitiveComponent* PrimitiveComponent)
		{
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			PrimitiveComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
			PrimitiveComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			PrimitiveComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
		});
	}	
#endif
}

#if WITH_EDITOR
void AMinimapWPHLOD::PreSave(FObjectPreSaveContext SaveContext)
{
	AActor::PreSave(SaveContext);
	
	// Epic Games! We need query collision!
	SetActorEnableCollision(true);

	ForEachComponent<UPrimitiveComponent>(false, [this, &SaveContext](UPrimitiveComponent* PrimitiveComponent)
	{
		// Epic Games! We need query collision!
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		// When cooking, get rid of collision data
		if (SaveContext.IsCooking())
		{
			if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(PrimitiveComponent))
			{
				if (UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh())
				{
					// If the HLOD process did create this static mesh
					if (StaticMesh->GetPackage() == GetPackage())
					{
						if (UBodySetup* BodySetup = StaticMesh->GetBodySetup())
						{
							// Epic Games! We need query collision!
							FGuid PreviousBodySetupGuid = BodySetup->BodySetupGuid;
							BodySetup->DefaultInstance.SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
							BodySetup->DefaultInstance.SetCollisionEnabled(ECollisionEnabled::QueryOnly);
							BodySetup->bNeverNeedsCookedCollisionData = false;
							BodySetup->bHasCookedCollisionData = true;
							BodySetup->BodySetupGuid = PreviousBodySetupGuid;
						}
					}
				}
			}
		}
	});
}
#endif

void AMinimapWPHLOD::SetVisibility(bool bIsVisible)
{
	Super::SetVisibility(bIsVisible);

	SetActorEnableCollision(bIsVisible);
	ForEachComponent<UPrimitiveComponent>(false, [this, bIsVisible](UPrimitiveComponent* PrimitiveComponent)
	{
		if (bIsVisible)
		{
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			PrimitiveComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
			PrimitiveComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			PrimitiveComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
		}
		else
		{
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	});

	// Manage child actors.
	TArray<AActor*> ChildActors;
	GetAttachedActors(ChildActors, true, true);
	for (auto ChildActor : ChildActors)
	{
		ChildActor->SetHidden(!bIsVisible);
		ChildActor->SetActorEnableCollision(bIsVisible);
		ChildActor->ForEachComponent<UPrimitiveComponent>(false, [this, bIsVisible](UPrimitiveComponent* PrimitiveComponent)
		{
			if (bIsVisible)
			{
				PrimitiveComponent->SetVisibility(bIsVisible);
				PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				PrimitiveComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
				PrimitiveComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
				PrimitiveComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
			}
			else
			{
				PrimitiveComponent->SetVisibility(bIsVisible);
				PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		});
	}
}
