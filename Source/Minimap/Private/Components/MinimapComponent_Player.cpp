// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/MinimapComponent_Player.h"

#include "EnhancedInputSubsystems.h"
#include "MinimapMapData.h"
#include "Actors/MapPinActor.h"
#include "MinimapSettings.h"
#include "MinimapZoneGraphAStar.h"
#include "Net/UnrealNetwork.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/MinimapGlobal.h"
#include "Engine/Canvas.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Serialization/ArchiveLoadCompressedProxy.h"
#include "Serialization/ArchiveSaveCompressedProxy.h"
#include "Widgets/MainMapUserWidget.h"
#include "Widgets/MinimapUserWidget.h"

void UMinimapComponent_Player::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMinimapComponent_Player, TempPinBrush);
	DOREPLIFETIME(UMinimapComponent_Player, TempPin);
	DOREPLIFETIME_CONDITION(UMinimapComponent_Player, PoiStateList, COND_OwnerOnly);
}

void UMinimapComponent_Player::ReceiveControllerChangedDelegate_Implementation(APawn* Pawn, AController* OldController,
	AController* NewController)
{
	ControllerChanged(NewController);
}

void UMinimapComponent_Player::ControllerChanged(const AController* NewController)
{
	// Avoid crashing!
	if (!GetWorld()) return;
	if (!GetWorld()->GetFirstLocalPlayerFromController()) return;

	const auto InputSubsystem = GetWorld()->GetFirstLocalPlayerFromController()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	
	if (!MinimapUserWidget && NewController && NewController->IsLocalPlayerController())
	{
		if (MinimapUserWidgetClass)
		{
			const auto LocalPlayerController = Cast<APlayerController>(OwnerPawn->GetController());
			MinimapUserWidget = CreateWidget<UMinimapUserWidget, APlayerController*>(LocalPlayerController, MinimapUserWidgetClass);
			MinimapUserWidget->LocalPawn = OwnerPawn;
			MinimapUserWidget->AddToViewport();
		}
		CreateAdditionalWidgets();
		if (InputSubsystem)
		{
			const auto Context = InputMappingContext.LoadSynchronous();
			InputSubsystem->AddMappingContext(Context, 0);
		}
	}
	// Vehicle update
	// else
	// {
	// 	if (MinimapUserWidget)
	// 	{
	// 		MinimapUserWidget->RemoveFromParent();
	// 		MinimapUserWidget = nullptr;
	// 	}
	// 	if (MainMapUserWidget)
	// 	{
	// 		MainMapUserWidget->RemoveFromParent();
	// 		MainMapUserWidget = nullptr;
	// 	}
	// 	RemoveAdditionalWidgets();
	// 	if (InputSubsystem)
	// 	{
	// 		const auto Context = InputMappingContext.LoadSynchronous();
	// 		InputSubsystem->RemoveMappingContext(Context);
	// 	}
	// }
}

UMinimapComponent_Player::UMinimapComponent_Player(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	
	TempPin = nullptr;
	bRotate = true;
	bAlwaysShow = true;

	OwnerPawn = nullptr;
	RT = nullptr;
	MaskLoadMaterial = nullptr;
	
	NavQueryStartPosition = FVector::Zero();
	NavQueryEndPosition = FVector::Zero();
}

void UMinimapComponent_Player::CreateRenderTarget()
{
	RT = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), Resolution, Resolution, RTF_RGBA16f, FLinearColor::Black, false, false);
}

void UMinimapComponent_Player::SetRenderTarget(const bool bInRenderTarget)
{
	bCreateRenderTarget = bInRenderTarget;
	if (bCreateRenderTarget && !RT)
	{
		CreateRenderTarget();
	}

	if (!bCreateRenderTarget && RT)
	{
		RT->ReleaseResource();
		RT = nullptr;
	}
}

void UMinimapComponent_Player::SetCurrentLocalMinimapData(UMinimapMapData* MinimapData)
{
	if (CurrentLocalMinimapData != MinimapData)
	{
		CurrentLocalMinimapData = MinimapData;
		OnLocalMinimapChanged.Broadcast(CurrentLocalMinimapData);
	}
}

//bool UMinimapComponent_Player::ShouldVisible_Implementation() const
//{
//	const auto OwnedPlayerController = OwnerPawn->GetController();
//	if (OwnedPlayerController == UGameplayStatics::GetPlayerController(GetWorld(), 0))
//	{
//		return true;
//	}
//	
//	auto SelfTeamID = Cast<IGenericTeamAgentInterface>(UGameplayStatics::GetPlayerController(GetWorld(), 0))->GetGenericTeamId();
//	if (!OwnerPawn->GetPlayerState())
//	{
//		return false;
//	}
//	auto CompTeamID = Cast<IGenericTeamAgentInterface>(OwnerPawn->GetPlayerState())->GetGenericTeamId();
//	
//	return SelfTeamID == CompTeamID;
//}

UMainMapUserWidget* UMinimapComponent_Player::GetOrCreateMainMapWidget()
{
	if (MainMapUserWidget)
	{
		return MainMapUserWidget;
	}
	
	if (OwnerPawn && OwnerPawn->IsLocallyControlled() && MainMapUserWidgetClass)
	{
		const auto LocalPlayerController = Cast<APlayerController>(OwnerPawn->GetController());
		MainMapUserWidget = CreateWidget<UMainMapUserWidget, APlayerController*>(LocalPlayerController, MainMapUserWidgetClass);
		MainMapUserWidget->LocalPawn = OwnerPawn;
	}

	return MainMapUserWidget;
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
		// Listen server fix
		if (GetOwnerRole() == ROLE_Authority)
		{
			AddTempPinImplement(HitResult.Location);
		}
		else
		{
			AddTempPinExec(HitResult.Location);
		}
	}
}

void UMinimapComponent_Player::RemoveTempPin_MainMap()
{
	RemoveTempPinExec();
}

void UMinimapComponent_Player::AddTempPinImplement(const FVector& Location)
{
	FActorSpawnParameters spawnInfo;
	spawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TempPin = GetWorld()->SpawnActorDeferred<AMapPinActor>(AMapPinActor::StaticClass(), FTransform(Location));
	if (TempPin)
	{
		TempPin->bCollision = true;
		TempPin->PinSlateBrush = TempPinBrush;
		TempPin->MinimapComp->bAlwaysShow = true;
		TempPin->SetOwner(GetOwner()->GetOwner());
		TempPin->FinishSpawning(FTransform(Location));
	}
}

void UMinimapComponent_Player::AddTempPinMulticast_Implementation(FVector Location)
{
	AddTempPinImplement(Location);
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
	PoiStateList.WorldContextObject = this;
	
	Super::BeginPlay();
	
	NavQueryPeriod = GetDefault<UMinimapSettings>()->NavQueryPeriod;
	OwnerPawn = Cast<APawn>(GetOwner());
	
	/** TODO : Unique color is not work when subsystem is LocalPlayerSubsystem
	if (GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		SetUniqueColorIndex();
	}
	*/
	
	// Add widget and input context.
	if (OwnerPawn)
	{
		if (OwnerPawn->IsLocallyControlled())
		{
			ControllerChanged(OwnerPawn->GetController());
		}
		else
		{
			OwnerPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &ThisClass::ReceiveControllerChangedDelegate);
		}
	}
	// Render target.
	if (bCreateRenderTarget)
	{
		RT = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), Resolution, Resolution, RTF_RGBA16f, FLinearColor::Black, false, false);
	}

	if (const auto Subsystem = GetWorld()->GetSubsystem<UMinimapSubsystem>())
	{
		Subsystem->OnMapPinAddEvent.AddDynamic(this, &ThisClass::OnMapPinAddEvent);
		Subsystem->OnMapPinRemoveEvent.AddDynamic(this, &ThisClass::OnMapPinRemoveEvent);
	}
}

void UMinimapComponent_Player::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (OwnerPawn)
	{
		if (const auto Controller = OwnerPawn->GetController())
		{
			if (Controller->IsLocalPlayerController())
			{
				UpdateNavPath(DeltaTime);
				UpdateMinimapShownPins();
			}
		}
	}
}

void UMinimapComponent_Player::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Remove input context and widget.
	ControllerChanged(nullptr);

	ShownMapPinsGuids.Empty();
}

void UMinimapComponent_Player::PostLoad()
{
	Super::PostLoad();
	OwnerPawn = Cast<APawn>(GetOwner());
}

void UMinimapComponent_Player::NativeGetDisplayNameAndDescription(FText& DisplayName, FText& Description) const
{
	if (OwnerPawn && OwnerPawn->GetPlayerState())
	{
		DisplayName = FText::FromString(OwnerPawn->GetPlayerState()->GetPlayerName());
	}
}

/** TODO : Unique color is not work when subsystem is LocalPlayerSubsystem
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
*/

void UMinimapComponent_Player::AddTempPinExec_Implementation(FVector Location)
{
	AddTempPinImplement(Location);
}

void UMinimapComponent_Player::RemoveTempPinExec_Implementation()
{
	if (TempPin)
	{
		TempPin->K2_DestroyActor();
		TempPin = nullptr;
	}
}

FMinimapSaveData UMinimapComponent_Player::GetSaveData() const
{
	FMinimapSaveData OutData;
	OutData.bHasTempPin = (TempPin != nullptr);
	if (TempPin != nullptr)
	{
		OutData.TempPinLocation = TempPin->GetActorLocation();
	}
	OutData.HotPointSaveGames = UMinimapFastArrayLibrary::GetPoiMapping(PoiStateList);
	return OutData;
}

void UMinimapComponent_Player::LoadSaveData(FMinimapSaveData inData, UTexture2D* MapMaskData)
{
	RemoveTempPinExec();
	if (inData.bHasTempPin)
	{
		AddTempPinExec(inData.TempPinLocation);
	}
	UMinimapFastArrayLibrary::LoadPoi(PoiStateList, inData.HotPointSaveGames);
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

bool UMinimapComponent_Player::HotPointCheck(FGuid Guid) const
{
	const auto Subsystem = GetWorld()->GetSubsystem<UMinimapSubsystem>();
	FPoiInfo OutInfo;
	bool bIsHotPoint = Subsystem->GetHotPointInfoFromGuid(Guid, OutInfo);
	if (!bIsHotPoint)
	{
		return true;
	}

	// TODO : Local minimap dont implement now.
	const auto bIsHotPointFound = IsHotPointFound(UGameplayStatics::GetCurrentLevelName(this), Guid);
	if (bIsHotPoint && !bIsHotPointFound)
	{
		return false;
	}

	return true;
}

TMap<FString, FMinimapIndices> UMinimapComponent_Player::GetFoundHotPoints() const
{
	if (const auto MinimapGlobal = GetWorld()->GetGameState()->GetComponentByClass<UMinimapGlobal>())
	{
		return PoiStateList.AppendOther(MinimapGlobal->PoiStateList);
	}
	
	return PoiStateList.PoiStateMap;
}

bool UMinimapComponent_Player::IsHotPointFound(const FString& LevelName, const FGuid& PoiIndex) const
{
	if (const auto MinimapGlobal = GetWorld()->GetGameState()->GetComponentByClass<UMinimapGlobal>())
	{
		if (UMinimapFastArrayLibrary::IsPoiFound(MinimapGlobal->PoiStateList, LevelName, PoiIndex))
		{
			return true;
		}
	}
	
	return UMinimapFastArrayLibrary::IsPoiFound(PoiStateList, LevelName, PoiIndex);
}

void UMinimapComponent_Player::FindHotPoint(const FString& LevelName, const FGuid& PoiIndex, bool Global)
{
	if (Global)
	{
		if (const auto MinimapGlobal = GetWorld()->GetGameState()->GetComponentByClass<UMinimapGlobal>())
		{
			UMinimapFastArrayLibrary::AddPoi(MinimapGlobal->PoiStateList, LevelName, PoiIndex);
		}
	}
	else
	{
		UMinimapFastArrayLibrary::AddPoi(PoiStateList, LevelName, PoiIndex);
	}
}

void UMinimapComponent_Player::SetMinimapRadius(const float Radius)
{
	MinimapRadius = Radius;
}

bool UMinimapComponent_Player::GetMinimapPinState(FGuid Guid, FMapPinStateEntry& OutEntry) const
{
	if (!Guid.IsValid())
	{
		OutEntry = FMapPinStateEntry();
		return false;
	}
	
	if (const auto Subsystem = GetWorld()->GetSubsystem<UMinimapSubsystem>())
	{
		if (Subsystem->GetMapPinCurrentState(Guid, OutEntry))
		{
			return true;
		}

		FPoiInfo OutInfo;
		if (Subsystem->GetHotPointInfoFromGuid(Guid, OutInfo))
		{
			OutEntry = static_cast<FMapPinStateEntry>(OutInfo);
			return true;
		}
	}
	
	OutEntry = FMapPinStateEntry();
	return false;
}

void UMinimapComponent_Player::UpdateMinimapShownPins()
{
	// Add pins guid and add always show pin
	const auto Subsystem = GetWorld()->GetSubsystem<UMinimapSubsystem>();
	
	TArray<FGuid> MapPinsGuidArray;
	const auto LocalList = Subsystem->GetLocalPinStateList();
	const auto GlobalList = Subsystem->GetGlobalPinStateList();
	TArray<FMapPinStateEntry> TotalEntries;
	TotalEntries.Append(LocalList.StateEntries);
	TotalEntries.Append(GlobalList.StateEntries);
	for (auto Entry : TotalEntries)
	{
		if (!Entry.bAlwaysOnMinimap)
		{
			MapPinsGuidArray.AddUnique(Entry.Id);
		}
		else
		{
			AddMinimapPin(Entry.Id);
		}
	}

	// Using quad tree to get found hot points.
	if (Subsystem && Subsystem->CurrentMinimapMapData)
	{
		const auto Data = Subsystem->CurrentMinimapMapData;
		const auto OutHotPoints = Data->QueryRange(GetOwner()->GetActorLocation(), MinimapRadius);
		MapPinsGuidArray.Append(OutHotPoints);
	}
	
	// Update visible
	for (const auto MapPin : MapPinsGuidArray)
	{
		FMapPinStateEntry OutEntry;
		if (GetMinimapPinState(MapPin, OutEntry))
		{
			if (FVector::Dist2D(GetOwner()->GetActorLocation(), OutEntry.Location) <= MinimapRadius / 2)
			{
				AddMinimapPin(MapPin);
			}
			else
			{
				RemoveMinimapPin(MapPin);
			}
		}
	}
}

void UMinimapComponent_Player::AddMinimapPin(FGuid Guid)
{
	if (ShownMapPinsGuids.Find(Guid) < 0 && HotPointCheck(Guid))
	{
		ShownMapPinsGuids.Add(Guid);
		OnMapPinShowOnMinimap.Broadcast(Guid);
	}
}

void UMinimapComponent_Player::RemoveMinimapPin(FGuid Guid)
{
	if (ShownMapPinsGuids.Find(Guid) >= 0)
	{
		OnMapPinHideOnMinimap.Broadcast(Guid);
		ShownMapPinsGuids.Remove(Guid);
	}
}

void UMinimapComponent_Player::OnMapPinAddEvent(const FGuid& MapPinId)
{
	FMapPinStateEntry OutEntry;
	if (GetMinimapPinState(MapPinId, OutEntry))
	{
		if (OutEntry.bAlwaysOnMinimap)
		{
			AddMinimapPin(MapPinId);
		}
	}
}

void UMinimapComponent_Player::OnMapPinRemoveEvent(const FGuid& MapPinId)
{
	RemoveMinimapPin(MapPinId);
}

bool UMinimapComponent_Player::ShouldShowNavPath() const
{
	return bShouldUpdateNavQuery && bPathPointsValid;
}

void UMinimapComponent_Player::UpdateNavPath(const float& DeltaTime)
{
	// Update end position.
	if (TempPin)
	{
		NavQueryEndPosition = TempPin->GetActorLocation();
		bShouldUpdateNavQuery = true;
	}
	else
	{
		bShouldUpdateNavQuery = false;
	}
	
	// Should update nav query.
	if (bShouldUpdateNavQuery)
	{
		// Update nav query start position using local player actor location.
		if (bAutoUpdateStartLocation)
		{
			NavQueryStartPosition = GetOwner()->GetActorLocation();
		}
		// Update nav query period. Using task to do async task update.
		NavQueryTime += DeltaTime;
		if (NavQueryTime >= NavQueryPeriod)
		{
			NavQueryTime = 0.0f;
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
			{
				FMinimapZoneGraphLanePath OutPath;
				bPathPointsValid = UMinimapZoneGraphAStarLibrary::GetZoneGraphPathBP(this, NavQueryStartPosition, NavQueryEndPosition, NavQueryExtend, OutPath);
				UMinimapZoneGraphAStarLibrary::GetPathPoints(this, OutPath, NavQueryOutPathPoints);
			});
		}
	}
}