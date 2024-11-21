// Fill out your copyright notice in the Description page of Project Settings.


#include "MapHotPointActor.h"

#include "Components/BillboardComponent.h"

AMapHotPointActor::AMapHotPointActor()
{
#if WITH_EDITORONLY_DATA
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	SetRootComponent(BillboardComponent);

	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinder<UTexture2D> BadTexture;
		FConstructorStatics()
			: BadTexture(TEXT("/Engine/EditorResources/Bad"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;
	DefaultTexture = ConstructorStatics.BadTexture.Object;
#endif
	
	PrimaryActorTick.bCanEverTick = false;
	SetIsSpatiallyLoaded(false);
}

void AMapHotPointActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Info.Location = GetActorLocation();
#if WITH_EDITORONLY_DATA
	if (Info.HotPointIcon)
	{
		BillboardComponent->SetSprite(Info.HotPointIcon);
	}
	else
	{
		BillboardComponent->SetSprite(DefaultTexture);
	}
#endif
}
