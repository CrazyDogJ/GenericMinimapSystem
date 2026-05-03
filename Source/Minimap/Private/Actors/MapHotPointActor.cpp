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
	if (!Info.IdentifyGuid.IsValid())
	{
		Info.IdentifyGuid = FGuid::NewGuid();
	}
}

void AMapHotPointActor::FoundThisMapHotPoint(UMinimapComponent_Player* PlayerComp, bool Global)
{
	if (!PlayerComp)
	{
		return;
	}

	PlayerComp->FindHotPoint(HotPointLevelName, Info.IdentifyGuid, Global);
}

void AMapHotPointActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Info.Location = GetActorLocation();
	
#if WITH_EDITOR
	if (!Info.HotPointId.IsEmpty())
	{
		Info.PinName = FText::ChangeKey(FTextKey("MinimapPOI"), Info.HotPointId + "Name", Info.PinName);
		Info.PinDescription = FText::ChangeKey(FTextKey("MinimapPOI"), Info.HotPointId + "Desc", Info.PinDescription);
	}
#endif
	
#if WITH_EDITORONLY_DATA
	if (Info.MapPinBrush.GetResourceObject())
	{
		if (const auto Tex = Cast<UTexture2D>(Info.MapPinBrush.GetResourceObject()))
		{
			BillboardComponent->SetSprite(Tex);
			BillboardComponent->ScreenSize = Info.MapPinBrush.ImageSize.X;
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

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FHotPointInfo, HotPointId))
	{
		if (!Info.HotPointId.IsEmpty())
		{
			Info.PinName = FText::ChangeKey(FTextKey("MinimapPOI"), Info.HotPointId + "Name", Info.PinName);
			Info.PinDescription = FText::ChangeKey(FTextKey("MinimapPOI"), Info.HotPointId + "Desc", Info.PinDescription);
		}
	}
	
	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FHotPointInfo, PinName))
	{
		if (!Info.HotPointId.IsEmpty())
		{
			Info.PinName = FText::ChangeKey(FTextKey("MinimapPOI"), Info.HotPointId + "Name", Info.PinName);
		}
	}

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FHotPointInfo, PinDescription))
	{
		if (!Info.HotPointId.IsEmpty())
		{
			Info.PinDescription = FText::ChangeKey(FTextKey("MinimapPOI"), Info.HotPointId + "Desc", Info.PinDescription);
		}
	}
}
#endif