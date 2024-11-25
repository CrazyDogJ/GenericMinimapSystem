// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapWPHLOD.h"

#include "PhysicsEngine/BodySetup.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionHelpers.h"

static int32 GWorldPartitionHLODForceDisableShadows = 0;
static FAutoConsoleVariableRef CVarWorldPartitionHLODForceDisableShadows(
	TEXT("wp.Runtime.HLOD.ForceDisableShadows"),
	GWorldPartitionHLODForceDisableShadows,
	TEXT("Force disable CastShadow flag on World Partition HLOD actors"),
	ECVF_Scalability);

AMinimapWPHLOD::AMinimapWPHLOD()
{
}

void AMinimapWPHLOD::PreRegisterAllComponents()
{
	AActor::PreRegisterAllComponents();
	
	if (GWorldPartitionHLODForceDisableShadows && GetWorld() && GetWorld()->IsGameWorld())
	{
		ForEachComponent<UPrimitiveComponent>(false, [](UPrimitiveComponent* PrimitiveComponent)
		{
			PrimitiveComponent->SetCastShadow(false);
		});
	}

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

	// If world is instanced, we need to recompute our bounds since they are in the instanced-world space
	if (UWorldPartition* WorldPartition = FWorldPartitionHelpers::GetWorldPartition(this))
	{
		const bool bIsInstancedLevel = WorldPartition->GetTypedOuter<ULevel>()->IsInstancedLevel();
		if (bIsInstancedLevel)
		{
			ForEachComponent<USceneComponent>(false, [](USceneComponent* SceneComponent)
			{
				// Clear bComputedBoundsOnceForGame so that the bounds are recomputed once
				SceneComponent->bComputedBoundsOnceForGame = false;
			});
		}
	}
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
							BodySetup->bNeverNeedsCookedCollisionData = true;
							BodySetup->bHasCookedCollisionData = false;
							BodySetup->InvalidatePhysicsData();
							BodySetup->BodySetupGuid = PreviousBodySetupGuid;
						}
					}
				}
			}
		}
	});
}
#endif