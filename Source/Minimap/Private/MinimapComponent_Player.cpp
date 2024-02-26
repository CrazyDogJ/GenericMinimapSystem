// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapComponent_Player.h"
#include "MinimapSettings.h"
#include "MinimapSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"

void UMinimapComponent_Player::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMinimapComponent_Player, TempPinBrush);
}

UMinimapComponent_Player::UMinimapComponent_Player(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bRotate = true;
	bAlwaysShow = true;

	OwnerPawn = nullptr;
}

void UMinimapComponent_Player::AddTempPin()
{
	FHitResult HitResult;
	FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(ClickableTrace), true );
	GetHitResultAtScreenPosition(UWidgetLayoutLibrary::GetViewportSize(GetWorld())/2, ECC_Visibility, CollisionQueryParams, HitResult);

	if (HitResult.IsValidBlockingHit())
	{
		if (!TempPin)
		{
			AddTempPinExec(HitResult.Location);
		}
		else
		{
			if (HitResult.GetActor() == TempPin)
			{
				RemoveTempPinExec();
			}
			else
			{
				RemoveTempPinExec();
				AddTempPinExec(HitResult.Location);
			}
		}
	}
}

void UMinimapComponent_Player::AddTempPin_MainMap(const FVector2D Location, const FMinimapStruct MapData, const ECollisionChannel TraceChannel)
{
	//Set map highest point, used to be the z location of the map capture actor.
	float MapHighestPoint = 100000.f;
	if (MapData.IsValid())
	{
		MapHighestPoint = MapData.CaptureActorLocation.Z;
	}

	//Get setting
	float HitResultTraceDistance = 100000.f;
	if (const UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>())
	{
		HitResultTraceDistance = Settings->ControllerHitResultDistance;
	}

	//Remove existed pin actor
	if (TempPin != nullptr)
	{
		RemoveTempPinExec();
	}
	
	//Line trace by channel, channel is visibility
	FHitResult HitResult;
	const FVector Start = FVector(Location.X, Location.Y, MapHighestPoint);
	const FVector End = FVector(Location.X, Location.Y, MapHighestPoint - HitResultTraceDistance);
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, TraceChannel))
	{
		AddTempPinExec(HitResult.Location);
	}
}

void UMinimapComponent_Player::RemoveTempPin_MainMap()
{
	RemoveTempPinExec();
}

bool UMinimapComponent_Player::GetHitResultAtScreenPosition(const FVector2D ScreenPosition,
                                                            const ECollisionChannel TraceChannel, const FCollisionQueryParams& CollisionQueryParams,
                                                            FHitResult& HitResult) const
{
	const APlayerController* controller = Cast<APawn>(GetOwner())->GetLocalViewingPlayerController();
	FVector WorldOrigin;
	FVector WorldDirection;
	if (UGameplayStatics::DeprojectScreenToWorld(controller, ScreenPosition, WorldOrigin, WorldDirection) == true)
	{
		float HitResultTraceDistance = 100000.f;
		if (const UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>())
		{
			HitResultTraceDistance = Settings->ControllerHitResultDistance;
		}
		return GetWorld()->LineTraceSingleByChannel(HitResult, WorldOrigin, WorldOrigin + WorldDirection * HitResultTraceDistance, TraceChannel, CollisionQueryParams);
	}

	return false;
}

void UMinimapComponent_Player::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		SetUniqueColorIndex();
	}
}

void UMinimapComponent_Player::PostLoad()
{
	Super::PostLoad();
	OwnerPawn = Cast<APawn>(GetOwner());
}

void UMinimapComponent_Player::SetUniqueColorIndex_Implementation()
{
	int32 loopIndex = -1;
	for (const auto comp : GetMinimapSubsystem()->GetRegisteredComponents())
	{
		if (comp->UniqueColorIndex > loopIndex)
		{
			loopIndex = comp->UniqueColorIndex;
		}
	}
	
	if (UniqueColorIndex == -1)
	{
		UniqueColorIndex = loopIndex+1;
	}

	if (UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>())
	{
		if (UniqueColorIndex < Settings->UniqueColors.Num())
		{
			PinSlateBrush.TintColor = Settings->UniqueColors[UniqueColorIndex];
			TempPinBrush.TintColor = Settings->UniqueColors[UniqueColorIndex];
		}
		else
		{
			TempPinBrush.TintColor = FLinearColor(128,0,128);
			PinSlateBrush.TintColor = FLinearColor(128,0,128);
		}
	}
	else
	{
		TempPinBrush.TintColor = FLinearColor(128,0,128);
		PinSlateBrush.TintColor = FLinearColor(128,0,128);
	}
}

void UMinimapComponent_Player::AddTempPinExec_Implementation(FVector Location)
{
	FActorSpawnParameters spawnInfo;
	spawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TempPin = GetWorld()->SpawnActorDeferred<AMapPinActor>(AMapPinActor::StaticClass(), FTransform(Location));
	if (TempPin)
	{
		TempPin->bCollision = true;
		TempPin->PinSlateBrush = TempPinBrush;
		TempPin->FinishSpawning(FTransform(Location));
	}
}

void UMinimapComponent_Player::RemoveTempPinExec_Implementation()
{
	if (TempPin)
	{
		TempPin->K2_DestroyActor();
		TempPin = nullptr;
	}
}

FMinimapSaveData UMinimapComponent_Player::GetSaveData()
{
	FMinimapSaveData OutData;
	OutData.bHasTempPin = (TempPin != nullptr);
	if (TempPin)
	{
		OutData.TempPinLocation = TempPin->GetActorLocation();
	}
	return OutData;
}

void UMinimapComponent_Player::LoadSaveData(FMinimapSaveData inData)
{
	RemoveTempPinExec();
	if (inData.bHasTempPin)
	{
		AddTempPinExec(inData.TempPinLocation);
	}
}