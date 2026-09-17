// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/MapHotPointActor.h"

#include "Components/BillboardComponent.h"

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
	if (!PoiInfo.HotPointId.IsValid())
	{
		PoiInfo.HotPointId = FGuid::NewGuid();
	}
}

void AMapHotPointActor::FoundThisMapHotPoint(APawn* Pawn, bool Global)
{
	if (!Pawn)
	{
		return;
	}

	// TODO : POI : Hot point found feature.
	// PlayerComp->FindHotPoint(HotPointLevelName, PoiInfo.HotPointId, Global);
}

void AMapHotPointActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	PoiInfo.Location = GetActorLocation();
	
#if WITH_EDITOR
	if (!PoiInfo.HotPointStringId.IsEmpty())
	{
		PoiInfo.DisplayName = FText::ChangeKey(FTextKey("MinimapPOI"), PoiInfo.HotPointStringId + "Name", PoiInfo.DisplayName);
		PoiInfo.DisplayDescription = FText::ChangeKey(FTextKey("MinimapPOI"), PoiInfo.HotPointStringId + "Desc", PoiInfo.DisplayDescription);
	}
#endif
	
#if WITH_EDITORONLY_DATA
	if (PoiInfo.Brush.GetResourceObject())
	{
		if (const auto Tex = Cast<UTexture2D>(PoiInfo.Brush.GetResourceObject()))
		{
			BillboardComponent->SetSprite(Tex);
			BillboardComponent->ScreenSize = PoiInfo.Brush.ImageSize.X;
		}
	}
	else
	{
		BillboardComponent->SetSprite(DefaultTexture);
	}
#endif
}

#if WITH_EDITOR
void AMapHotPointActor::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FPoiInfo, HotPointStringId))
	{
		if (!PoiInfo.HotPointStringId.IsEmpty())
		{
			PoiInfo.DisplayName = FText::ChangeKey(FTextKey("MinimapPOI"), PoiInfo.HotPointStringId + "Name", PoiInfo.DisplayName);
			PoiInfo.DisplayDescription = FText::ChangeKey(FTextKey("MinimapPOI"), PoiInfo.HotPointStringId + "Desc", PoiInfo.DisplayDescription);
		}
	}
	
	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FPoiInfo, DisplayName))
	{
		if (!PoiInfo.HotPointStringId.IsEmpty())
		{
			PoiInfo.DisplayName = FText::ChangeKey(FTextKey("MinimapPOI"), PoiInfo.HotPointStringId + "Name", PoiInfo.DisplayName);
		}
	}

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FPoiInfo, DisplayDescription))
	{
		if (!PoiInfo.HotPointStringId.IsEmpty())
		{
			PoiInfo.DisplayDescription = FText::ChangeKey(FTextKey("MinimapPOI"), PoiInfo.HotPointStringId + "Desc", PoiInfo.DisplayDescription);
		}
	}
}
#endif