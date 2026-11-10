// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/MapPinActor.h"

#include "MinimapPinObject.h"
#include "MinimapSubsystem.h"
#include "NetRelevantGlobalComponent.h"
#include "NetRelevantObjectUtils.h"
#include "Components/MinimapGlobal.h"
#include "Net/UnrealNetwork.h"

void AMapPinActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, OwningController);
}

AMapPinActor::AMapPinActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneComponent);
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
}

void AMapPinActor::OnRep_OwningController()
{
	if (!OwningController)
	{
		return;
	}
	
	if (OwningController->IsLocalPlayerController())
	{
		if (const auto MinimapSubsystem = GetWorld()->GetSubsystem<UMinimapSubsystem>())
		{
			MinimapSubsystem->LocalTempPinActor = this;
		}
	}
}

void AMapPinActor::Overlapped(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,int32 OtherBodyIndex,bool bFromSweep,const FHitResult& SweepResult)
{
	// Avoid nullptr;
	if (!OtherActor) return;
	
	// Main function, only work for owning controller's pawn.
	const auto ShapeComponent = Cast<UShapeComponent>(OtherComp);
	// 修复：触发重叠的组件也可能是网格，或其 Actor 正在销毁。
	if (!ShapeComponent || !IsValid(ShapeComponent->GetOwner())) return;
	const auto IsRoot = ShapeComponent->GetOwner()->GetRootComponent() == ShapeComponent;
	const auto IsPawn = Cast<APawn>(ShapeComponent->GetOwner());
	if (OwningController && OwningController->GetPawn() == OtherActor && IsRoot && IsPawn)
	{
		K2_DestroyActor();
	}
}

void AMapPinActor::OnControllerDestroyed(AActor* DestroyedActor)
{
	K2_DestroyActor();
}

// Called when the game starts or when spawned
void AMapPinActor::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeOwningController();
	// 修复：缺少控制器时初始化会销毁标记，不再创建关联对象。
	if (IsActorBeingDestroyed()) return;
	AddMapPin();
	InitializeCollision();
}

bool AMapPinActor::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget,
	const FVector& SrcLocation) const
{
	if (!OwningController)
	{
		return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
	}
	
	const auto ViewerGroups = UNetRelevantObjectFunctionLibrary::GetPlayerNetGroups(Cast<APlayerController>(RealViewer));
	const auto ActorGroups = UNetRelevantObjectFunctionLibrary::GetPlayerNetGroups(Cast<APlayerController>(OwningController));
	return UNetRelevantObjectFunctionLibrary::CanPlayerReceiveSubobject(ActorGroups, ViewerGroups);
}

void AMapPinActor::AddMapPin()
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (const auto Global = UNetRelevantObjectFunctionLibrary::GetNetRelevantGlobalComponent(this))
	{
		const auto Object = Global->AddNetRelevantObject<UMinimapPinObject>(Cast<APlayerController>(OwningController), GLOBAL_GROUP);
		if (!Object) return;
		Object->ReplicatingData.AttachedComponent = GetRootComponent();
		Object->ReplicatingData.Brush = PinSlateBrush;
		Object->ReplicatingData.bAddToOverlay = true;
		Object->ReplicatingData.bHasYaw = false;
		Object->ReplicatingData.bAlwaysOnMinimap = true;
		if (const auto MinimapGlobal = Cast<UMinimapGlobal>(Global))
		{
			Object->ReplicatingData.CustomMainmapWidgetClass = MinimapGlobal->TempPinMainMapWidgetClass;
		}
		Global->BeginObjectLogic(Object);
		Id = Object->Id;
	}
}

void AMapPinActor::InitializeOwningController()
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (!OwningController)
	{
		K2_DestroyActor();
	}
	else
	{
		OwningController->OnDestroyed.AddDynamic(this, &AMapPinActor::OnControllerDestroyed);
		OnRep_OwningController();
	}
}

void AMapPinActor::InitializeCollision()
{
	if (bCollision)
	{
		const FTransform emptyTransform;
		SphereCollision = Cast<USphereComponent>(AddComponentByClass(USphereComponent::StaticClass(), false, emptyTransform, false));
		if (!SphereCollision) return;
		SphereCollision->SetCollisionResponseToAllChannels(ECR_Overlap);
		SphereCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		SphereCollision->SetGenerateOverlapEvents(true);
		SphereCollision->SetSphereRadius(CollisionRadius);
		SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &AMapPinActor::Overlapped);
	}
}

void AMapPinActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (HasAuthority())
	{
		if (const auto Global = UNetRelevantObjectFunctionLibrary::GetNetRelevantGlobalComponent(this))
		{
			Global->RemoveNetRelevantObject(Id);
		}
	}
	
	if (OwningController && OwningController->IsLocalPlayerController())
	{
		if (const auto MinimapSubsystem = GetWorld()->GetSubsystem<UMinimapSubsystem>())
		{
			if (MinimapSubsystem->LocalTempPinActor == this) MinimapSubsystem->LocalTempPinActor = nullptr;
		}
	}
}

void AMapPinActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	AddActorLocalOffset(FVector(0,0,50));
}
