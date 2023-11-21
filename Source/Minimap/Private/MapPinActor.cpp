// Fill out your copyright notice in the Description page of Project Settings.


#include "MapPinActor.h"

#include "MapPinWidget.h"
#include "MinimapComponent.h"
#include "MinimapSettings.h"
#include "Net/UnrealNetwork.h"

void AMapPinActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMapPinActor, PinSlateBrush);
}

AMapPinActor::AMapPinActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneComponent);
	MinimapComp = CreateDefaultSubobject<UMinimapComponent>(TEXT("MinimapComponent"));
	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	WidgetComponent->AttachToComponent(SceneComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
}

void AMapPinActor::Overlapped(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,int32 OtherBodyIndex,bool bFromSweep,const FHitResult& SweepResult)
{
	if (Cast<UMinimapComponent>(OtherActor->GetComponentByClass(UMinimapComponent::StaticClass()))->TempPin == this)
	{
		Cast<UMinimapComponent>(OtherActor->GetComponentByClass(UMinimapComponent::StaticClass()))->TempPin = nullptr;
		K2_DestroyActor();
	}
}

// Called when the game starts or when spawned
void AMapPinActor::BeginPlay()
{
	Super::BeginPlay();
	UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>();
	Settings->LoadConfig(UMinimapSettings::StaticClass());
	if (Settings->CommonMapPinWidget->IsValidLowLevel())
	{
		if (UMapPinWidget* Widget = CreateWidget<UMapPinWidget>(GetWorld(), Settings->CommonMapPinWidget))
		{
			Widget->Brush = PinSlateBrush;
			Widget->OwnerActor = this;
			WidgetComponent->SetWidget(Widget);
			WidgetComponent->RequestRedraw();
		}
	}
	
	if (bCollision)
	{
		const FTransform emptyTransform;
		SphereCollision = Cast<USphereComponent>(AddComponentByClass(USphereComponent::StaticClass(), false, emptyTransform, false));
		SphereCollision->SetCollisionResponseToAllChannels(ECR_Overlap);
		SphereCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		SphereCollision->SetGenerateOverlapEvents(true);
		SphereCollision->SetSphereRadius(CollisionRadius);
		SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &AMapPinActor::Overlapped);
	}
}

void AMapPinActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	MinimapComp->DestroyComponent();
	MinimapComp = nullptr;
	WidgetComponent->DestroyComponent();
	
	Super::EndPlay(EndPlayReason);
}

void AMapPinActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	MinimapComp->PinSlateBrush = PinSlateBrush;
	MinimapComp->bRotate = false;
	MinimapComp->bAlwaysShow = true;
	AddActorLocalOffset(FVector(0,0,50));
}
