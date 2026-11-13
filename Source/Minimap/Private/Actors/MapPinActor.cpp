// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/MapPinActor.h"

#include "MinimapPinObject.h"
#include "MinimapSubsystem.h"
#include "NetRelevantGlobalComponent.h"
#include "NetRelevantObjectUtils.h"
#include "Components/MinimapGlobal.h"
#include "Net/UnrealNetwork.h"
#if UE_WITH_IRIS && ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
#include "Engine/NetConnection.h"
#include "Iris/ReplicationSystem/ObjectReplicationBridge.h"
#include "Iris/ReplicationSystem/ReplicationSystem.h"
#include "Iris/ReplicationSystem/Filtering/NetObjectFilter.h"
#include "Net/Iris/ReplicationSystem/ReplicationSystemUtil.h"
#endif

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
	SetReplicateMovement(true);
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
	if (!HasAuthority()) return;
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

#if UE_WITH_IRIS && ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
void AMapPinActor::OnReplicationStartedForIris(const FOnReplicationStartedParams& Params)
{
	Super::OnReplicationStartedForIris(Params);
	using namespace UE::Net;
	auto* System = FReplicationSystemUtil::GetReplicationSystem(this);
	if (!System || !System->IsServer()) return;
	const auto Handle = System->GetReplicationBridge()->GetReplicatedRefHandle(this);
	if (!Handle.IsValid()) return;
	const auto Group = System->CreateGroup(NAME_None);
	if (!Group.IsValid() || !System->AddExclusionFilterGroup(Group))
	{
		if (Group.IsValid()) System->DestroyGroup(Group);
		System->AddToGroup(System->GetNotReplicatedNetObjectGroup(), Handle);
		return;
	}
	System->AddToGroup(Group, Handle);
	// A teammate can see a map marker at any distance.
	System->SetFilter(Handle, InvalidNetObjectFilterHandle);
	IrisViewerGroup = Group;
	IrisSystem = System;
	IrisAllowedConnections.Init(System->GetMaxConnectionCount());
	IrisNextConnections.Init(System->GetMaxConnectionCount());
	UpdateIrisViewers(0.f);
	IrisFlushHandle = GetWorld()->OnPreTickFlush().AddUObject(this, &ThisClass::UpdateIrisViewers);
}

void AMapPinActor::UpdateIrisViewers(float DeltaSeconds)
{
	using namespace UE::Net;
	auto* System = IrisSystem.Get();
	if (!System) return;
	const auto Groups = UNetRelevantObjectFunctionLibrary::GetPlayerNetGroups(Cast<APlayerController>(OwningController));
	IrisNextConnections.ClearAllBits();
	for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		auto* PC = It->Get();
		auto* Connection = PC ? PC->GetNetConnection() : nullptr;
		if (!Connection) continue;
		const uint32 ConnectionId = Connection->GetConnectionHandle().GetParentConnectionId();
		if (ConnectionId < IrisNextConnections.GetNumBits() &&
			UNetRelevantObjectFunctionLibrary::CanPlayerReceiveSubobject(Groups, PC->GetNetConditionGroups()))
			IrisNextConnections.SetBit(ConnectionId);
	}
	if (IrisNextConnections != IrisAllowedConnections)
	{
		System->SetGroupFilterStatus(IrisViewerGroup, IrisNextConnections, ENetFilterStatus::Allow);
		Swap(IrisAllowedConnections, IrisNextConnections);
	}
}

void AMapPinActor::OnStopReplicationForIris(const FOnStopReplicationParams& Params)
{
	ReleaseIrisViewerGroup();
	Super::OnStopReplicationForIris(Params);
}

void AMapPinActor::ReleaseIrisViewerGroup()
{
	if (IrisFlushHandle.IsValid())
	{
		GetWorld()->OnPreTickFlush().Remove(IrisFlushHandle);
		IrisFlushHandle.Reset();
	}
	if (auto* System = IrisSystem.Get()) System->DestroyGroup(IrisViewerGroup);
	IrisSystem.Reset();
	IrisViewerGroup = {};
	IrisAllowedConnections.Empty();
	IrisNextConnections.Empty();
}
#endif

void AMapPinActor::AddMapPin()
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (const auto Global = UNetRelevantObjectFunctionLibrary::GetNetRelevantGlobalComponent(this))
	{
		const auto Object = Cast<UMinimapPinObject>(Global->AddNetRelevantObjectFollowingOwnerGroups(
			UMinimapPinObject::StaticClass(), Cast<APlayerController>(OwningController)));
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
#if UE_WITH_IRIS && ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
	ReleaseIrisViewerGroup();
#endif
	Super::EndPlay(EndPlayReason);

	if (HasAuthority())
	{
		if (const auto Global = UNetRelevantObjectFunctionLibrary::GetNetRelevantGlobalComponent(this))
		{
			if (auto* Minimap = Cast<UMinimapGlobal>(Global); Minimap && Minimap->TempMapPinMapping.FindRef(OwningController) == this)
				Minimap->TempMapPinMapping.Remove(OwningController);
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
