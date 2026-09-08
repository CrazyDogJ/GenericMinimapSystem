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
	if (!PoiInfo.Id.IsValid())
	{
		PoiInfo.Id = FGuid::NewGuid();
	}
}

void AMapHotPointActor::FoundThisMapHotPoint(UMinimapComponent_Player* PlayerComp, bool Global)
{
	if (!PlayerComp)
	{
		return;
	}

	PlayerComp->FindHotPoint(HotPointLevelName, PoiInfo.Id, Global);
}

void AMapHotPointActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	PoiInfo.ActorWeakPtr = this;
	PoiInfo.Location = GetActorLocation();
	
#if WITH_EDITOR
	if (!PoiInfo.HotPointId.IsEmpty())
	{
		PoiInfo.PinName = FText::ChangeKey(FTextKey("MinimapPOI"), PoiInfo.HotPointId + "Name", PoiInfo.PinName);
		PoiInfo.PinDescription = FText::ChangeKey(FTextKey("MinimapPOI"), PoiInfo.HotPointId + "Desc", PoiInfo.PinDescription);
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

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FPoiInfo, HotPointId))
	{
		if (!PoiInfo.HotPointId.IsEmpty())
		{
			PoiInfo.PinName = FText::ChangeKey(FTextKey("MinimapPOI"), PoiInfo.HotPointId + "Name", PoiInfo.PinName);
			PoiInfo.PinDescription = FText::ChangeKey(FTextKey("MinimapPOI"), PoiInfo.HotPointId + "Desc", PoiInfo.PinDescription);
		}
	}
	
	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FPoiInfo, PinName))
	{
		if (!PoiInfo.HotPointId.IsEmpty())
		{
			PoiInfo.PinName = FText::ChangeKey(FTextKey("MinimapPOI"), PoiInfo.HotPointId + "Name", PoiInfo.PinName);
		}
	}

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FPoiInfo, PinDescription))
	{
		if (!PoiInfo.HotPointId.IsEmpty())
		{
			PoiInfo.PinDescription = FText::ChangeKey(FTextKey("MinimapPOI"), PoiInfo.HotPointId + "Desc", PoiInfo.PinDescription);
		}
	}
}
#endif