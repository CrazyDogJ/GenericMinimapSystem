// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/MapHotPointActor.h"

#include "Components/BillboardComponent.h"
#include "Kismet/GameplayStatics.h"

AMapHotPointActor::AMapHotPointActor()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
#if WITH_EDITORONLY_DATA
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	BillboardComponent->SetupAttachment(Root);

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
	if (!Info.IdentifyGuid.IsValid())
	{
		Info.IdentifyGuid = FGuid::NewGuid();
	}
}

void AMapHotPointActor::FoundThisMapHotPoint(UMinimapComponent_Player* PlayerComp)
{
	if (!PlayerComp)
	{
		return;
	}

	FHotPointSaveGame* FoundStruct = PlayerComp->HotPointSaveGames.FindByPredicate([&](const FHotPointSaveGame& Item)
	{
		return Item.LevelName == UGameplayStatics::GetCurrentLevelName(GetWorld());
	});
	
	if (FoundStruct)
	{
		FoundStruct->HotPointFoundMap.Add(Info.IdentifyGuid, true);
	}
	else
	{
		auto NewStruct = PlayerComp->HotPointSaveGames.Add(FHotPointSaveGame());
		PlayerComp->HotPointSaveGames[NewStruct].LevelName = UGameplayStatics::GetCurrentLevelName(GetWorld());
		PlayerComp->HotPointSaveGames[NewStruct].HotPointFoundMap.Add(Info.IdentifyGuid, true);
	}
}

void AMapHotPointActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Info.Location = GetActorLocation();
#if WITH_EDITORONLY_DATA
	if (Info.MapPinBrush.GetResourceObject())
	{
		if (const auto Tex = Cast<UTexture2D>(Info.MapPinBrush.GetResourceObject()))
		{
			BillboardComponent->SetSprite(Tex);
		}
	}
	else
	{
		BillboardComponent->SetSprite(DefaultTexture);
	}
#endif
}
