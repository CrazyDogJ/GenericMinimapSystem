// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapComponent_Player.h"

#include "GenericTeamAgentInterface.h"
#include "MinimapSettings.h"
#include "MinimapSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Serialization/ArchiveLoadCompressedProxy.h"
#include "Serialization/ArchiveSaveCompressedProxy.h"

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
	RT = nullptr;
	MaskLoadMaterial = nullptr;
}

bool UMinimapComponent_Player::ShouldVisible() const
{
	const auto OwnedPlayerController = OwnerPawn->GetController();
	if (OwnedPlayerController == UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		return true;
	}
	
	auto SelfTeamID = Cast<IGenericTeamAgentInterface>(UGameplayStatics::GetPlayerController(GetWorld(), 0))->GetGenericTeamId();
	if (!OwnerPawn->GetPlayerState())
	{
		return false;
	}
	auto CompTeamID = Cast<IGenericTeamAgentInterface>(OwnerPawn->GetPlayerState())->GetGenericTeamId();
	
	return SelfTeamID == CompTeamID;
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

void UMinimapComponent_Player::AddTempPin_MainMap(const FVector2D Location, const UMinimapMapData* MapData, const ECollisionChannel TraceChannel)
{
	//Set map highest point, used to be the z location of the map capture actor.
	float MapHighestPoint = 100000.f;
	if (MapData)
	{
		MapHighestPoint = MapData->CaptureActorLocation.Z;
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

TArray<uint8> UMinimapComponent_Player::SerializeRenderTargetData(int32& Width, int32& Height) const
{
	if (!RT)
	{
		return TArray<uint8>();
	}

	Width = RT->SizeX;
	Height = RT->SizeY;
	
	TArray<FColor> PixelData;
	if (FRenderTarget* RenderTarget = RT->GameThread_GetRenderTargetResource())
	{
		RenderTarget->ReadPixels(PixelData);
	}

	TArray<uint8> SerializedData;
	FMemoryWriter Writer(SerializedData, true);
	Writer << PixelData;
	
	TArray<uint8> CompressedData;
	FArchiveSaveCompressedProxy Compressor(CompressedData, NAME_Zlib);
	Compressor << SerializedData;
	Compressor.Flush();
	
	return CompressedData;
}

void UMinimapComponent_Player::SendRenderTargetData()
{
	int32 Width;
	int32 Height;
	auto SerializedData = SerializeRenderTargetData(Width, Height);
	if (GetOwnerRole() >= ROLE_Authority)
	{
		SendRenderTargetToClients(Width, Height, SerializedData);
		return;
	}

	SendRenderTargetToServer(Width, Height, SerializedData);
}

void UMinimapComponent_Player::SendRenderTargetToServer_Implementation(const int& Width, const int& Height, const TArray<uint8>& Data)
{
	SendRenderTargetToClients(Width, Height, Data);
}

void UMinimapComponent_Player::SendRenderTargetToClients_Implementation(const int& Width, const int& Height, const TArray<uint8>& Data)
{
	TArray<uint8> DecompressedData;
	FArchiveLoadCompressedProxy Decompressor(Data, NAME_Zlib);
	Decompressor << DecompressedData;

	TArray<FColor> PixelData;
	FMemoryReader Reader(DecompressedData, true);
	Reader << PixelData;

	if (UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8))
	{
		FTexture2DMipMap& Mip = NewTexture->GetPlatformData()->Mips[0];
		void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(TextureData, PixelData.GetData(), PixelData.Num() * sizeof(FColor));
		Mip.BulkData.Unlock();
		NewTexture->UpdateResource();
		
		OnRenderTargetReceived(NewTexture);
	}
}

void UMinimapComponent_Player::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		SetUniqueColorIndex();
	}

	RT = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), Resolution, Resolution, RTF_RGBA16f, FLinearColor::Black, false, false);
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
	if (TempPin != nullptr)
	{
		OutData.TempPinLocation = TempPin->GetActorLocation();
	}
	OutData.HotPointSaveGames = HotPointSaveGames;
	return OutData;
}

void UMinimapComponent_Player::LoadSaveData(FMinimapSaveData inData, UTexture2D* MapMaskData)
{
	RemoveTempPinExec();
	if (inData.bHasTempPin)
	{
		AddTempPinExec(inData.TempPinLocation);
	}
	HotPointSaveGames = inData.HotPointSaveGames;
	if (RT)
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), RT, FLinearColor::Black);
		UCanvas* Canvas = nullptr;
		FVector2D Size;
		FDrawToRenderTargetContext Context;
		UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), RT, Canvas, Size, Context);
		auto BrushMaterial = UKismetMaterialLibrary::CreateDynamicMaterialInstance(GetWorld(), MaskLoadMaterial);
		BrushMaterial->SetTextureParameterValue(TexturePropertyName, MapMaskData);
		Canvas->K2_DrawMaterial(BrushMaterial, FVector2D(0,0), FVector2D(Resolution, Resolution), FVector2D(0,0));
		UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
	}
}

bool UMinimapComponent_Player::IsHotPointFound(FHotPointInfo HotPointInfo)
{
	FHotPointSaveGame* FoundStruct = HotPointSaveGames.FindByPredicate([&](const FHotPointSaveGame& Item)
	{
		return Item.LevelName == UGameplayStatics::GetCurrentLevelName(GetWorld());
	});

	if (FoundStruct)
	{
		if (const auto Ptr = FoundStruct->HotPointFoundMap.Find(HotPointInfo.HotPointUniqueID))
		{
			return *Ptr;
		}
	}
	
	return false;
}
