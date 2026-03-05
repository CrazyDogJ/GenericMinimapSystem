// Fill out your copyright notice in the Description page of Project Settings.


#include "MapCaptureActor.h"

#include "ImageUtils.h"
#include "Actors/MapHotPointActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "MinimapSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "UObject/SavePackage.h"

// Sets default values
AMapCaptureActor::AMapCaptureActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	SetRootComponent(SceneComponent);
	Capture2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture2D"));
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	Capture2D->ProjectionType = ECameraProjectionMode::Orthographic;
	Capture2D->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0, -90, 0)));
	Capture2D->bCaptureEveryFrame = false;
	Capture2D->bCaptureOnMovement = false;
	bIsEditorOnlyActor = true;
	SceneComponent->bIsEditorOnly = true;
	Capture2D->bIsEditorOnly = true;
	Capture2D->TextureTarget = nullptr;
}

bool ReadRenderTargetPixels(UTextureRenderTarget2D* RT, TArray<FColor>& OutPixels)
{
	if (!RT) return false;

	FTextureRenderTargetResource* RTResource = RT->GameThread_GetRenderTargetResource();
	if (!RTResource) return false;
	
	return RTResource->ReadPixels(OutPixels);
}

void CopyTileToFinal(const FTileCaptureResult& Tile, int32 FinalSize, TArray<FColor>& FinalPixels)
{
	const int32 TileSize = Tile.Size;

	for (int32 Row = 0; Row < TileSize; Row++)
	{
		int32 DestY = Tile.TileY * TileSize + Row;
		int32 SrcY = Row;

		FColor* DestPtr = &FinalPixels[DestY * FinalSize + Tile.TileX * TileSize];
		const FColor* SrcPtr = &Tile.Pixels[SrcY * TileSize];

		FMemory::Memcpy(DestPtr, SrcPtr, TileSize * sizeof(FColor));
	}
}

void StitchTiles(const TArray<FTileCaptureResult>& Tiles, int32 TilesPerAxis, int32 TileSize,
	TArray<FColor>& OutPixels, int32& OutSize)
{
	OutSize = TilesPerAxis * TileSize;
	OutPixels.SetNumZeroed(OutSize * OutSize);

	for (const FTileCaptureResult& Tile : Tiles)
	{
		CopyTileToFinal(Tile, OutSize, OutPixels);
	}
}

void AMapCaptureActor::CaptureMap()
{
	TilePositions.Empty();
	
	// Setup something.
	const int32 TilesPerAxis = TextureScale / TileSize;
	const float TileWorldSize = EndPoint.X / TilesPerAxis;

	// Loop capture.
	Capture2D->OrthoWidth = EndPoint.X / TilesPerAxis;
	for (int32 y = 0; y < TilesPerAxis; y++)
	{
		for (int32 x = 0; x < TilesPerAxis; x++)
		{
			FVector WorldPos = GetActorLocation()
				+ GetActorRightVector() * (x + 0.5f) * TileWorldSize
				+ GetActorForwardVector() * (y + 0.5f) * TileWorldSize;

			TilePositions.Add(FIntPoint(x, y), WorldPos);
		}
	}

	StartCapture();
}

void AMapCaptureActor::SaveMapInfo(UMinimapMapData* NewDataAsset, const FString& Path, const FString& Name)
{
	if (NewDataAsset)
	{
		if (UPackage* Package = CreatePackage(*Path))
		{
			NewDataAsset->Rename(*Name, Package);

			FAssetRegistryModule::AssetCreated(NewDataAsset);
			FString PackageFilePath = FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension());
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;
			UPackage::SavePackage(Package, NewDataAsset, *PackageFilePath, SaveArgs);

			//UE_LOG(LogTemp, Log, TEXT("Data Asset created and saved successfully at: %s"), *Path);
		}
	}
}

void AMapCaptureActor::WriteMapInfo(UMinimapMapData* DataAsset, UTexture2D* Tex)
{
	DataAsset->LevelName = MapName;
	DataAsset->MapSize = EndPoint.X;
	DataAsset->MapTexture = Tex;
	DataAsset->TextureSize = TextureScale;
	DataAsset->CaptureActorLocation = GetActorLocation();
	DataAsset->HotPointInfos.Empty();
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMapHotPointActor::StaticClass(), OutActors);
	for (auto Actor : OutActors)
	{
		if (auto Point = Cast<AMapHotPointActor>(Actor))
		{
			DataAsset->HotPointInfos.Add(Point->Info);
		}
	}
}

void AMapCaptureActor::StartCapture()
{
	AllTiles.Empty();
	CurrentTileIndex = 0;
	CaptureNextTile();
}

void AMapCaptureActor::CaptureNextTile()
{
	const int32 TilesPerAxis = TextureScale / TileSize;
	const auto Array = TilePositions.Array();
	if (Array.IsValidIndex(CurrentTileIndex))
	{
		const auto TilePos = Array[CurrentTileIndex];
		Capture2D->SetWorldLocation(TilePos.Value);
		Capture2D->CaptureScene();
			
		FTileCaptureResult Tile;
		Tile.TileX = TilePos.Key.X;
		Tile.TileY = TilesPerAxis - 1 - TilePos.Key.Y;
		Tile.Size = TileSize;

		ReadRenderTargetPixels(Capture2D->TextureTarget, Tile.Pixels);
		AllTiles.Add(Tile);
	}
	
	CurrentTileIndex++;
	if (CurrentTileIndex == Array.Num())
	{
		CaptureFinished();
	}
	else
	{
		CaptureNextTile();
		// GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::CaptureNextTile);
	}
}

void AMapCaptureActor::CaptureFinished()
{
	const int32 TilesPerAxis = TextureScale / TileSize;
	
	// Stitch
	TArray<FColor> FinalPixels;
	int32 FinalSize;
	StitchTiles(AllTiles, TilesPerAxis, TileSize, FinalPixels, FinalSize);

	// Get texture name.
	FString TexName = FString(TEXT("T_")) + MapName;
	UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>();
	FString TotalFileName = FPaths::Combine(Settings->MapTexturePath, TexName);

	// Get path and try to delete outdated map texture.
	// FAssetRegistryModule* const AssetRegistryModule = FModuleManager::Get().GetModulePtr<FAssetRegistryModule>("AssetRegistry");
	// if (!AssetRegistryModule)
	// {
	// 	return;
	// }
	// const IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
	// TArray<FAssetData> outData;
	// AssetRegistry.GetAssetsByPath(FName(*Settings->MapTexturePath), outData);
	// for (auto data : outData)
	// {
	// 	if (data.AssetName.ToString() == TexName)
	// 	{
	// 		UEditorAssetLibrary::DeleteAsset(TotalFileName);
	// 	}
	// }

	FCreateTexture2DParameters CreateTexture2DParameters = FCreateTexture2DParameters();
	
	// Find or create package.
	UPackage* MapTexturePackage = CreatePackage(*TotalFileName);
	MapTexturePackage->FullyLoad();
	MapTexturePackage->Modify();
	
	// Create texture object.
	UTexture2D* NewTexture = FImageUtils::CreateTexture2D(
		FinalSize,
		FinalSize,
		FinalPixels,
		MapTexturePackage,
		TexName,
		RF_Public | RF_Standalone,
		CreateTexture2DParameters
	);
	
	FAssetRegistryModule::AssetCreated(NewTexture);
 
	// ReSharper disable once CppExpressionWithoutSideEffects
	MapTexturePackage->MarkPackageDirty();
 
	if (GEditor)
	{
		UEditorAssetSubsystem* AssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
		AssetSubsystem->SaveLoadedAsset(NewTexture);
	}
	
	if (auto Value = Settings->MapsInfos.Find(MapName))
	{
		if (UMinimapMapData* MapData = Value->LoadSynchronous())
		{
			WriteMapInfo(MapData, NewTexture);
			// ReSharper disable once CppExpressionWithoutSideEffects
			MapData->MarkPackageDirty();
		}
	}
	else
	{
		Settings->LoadConfig(UMinimapSettings::StaticClass(), *Settings->GetDefaultConfigFilename());
		FString AssetPath = Settings->MapTexturePath + "DA_" + MapName;
		FString AssetName = "DA_" + MapName;
		UPackage* Package = CreatePackage(*AssetPath);
		if (UMinimapMapData* NewMapInfo = NewObject<UMinimapMapData>(Package, *AssetName, RF_Public | RF_Standalone))
		{
			WriteMapInfo(NewMapInfo, NewTexture);
		}
		// save mapper class
		FString const PackageName = Package->GetName();
		FString const PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs);
		
		auto SoftRef = TSoftObjectPtr<UMinimapMapData>(FSoftObjectPath(AssetPath + "." + AssetName));
		if (!bLocalMap)
		{
			Settings->MapsInfos.Add(UGameplayStatics::GetCurrentLevelName(GetWorld()), SoftRef);
			Settings->SaveConfig(CPF_Config, *Settings->GetDefaultConfigFilename());
		}
	}

	AllTiles.Empty();
}

void AMapCaptureActor::OnConstruction(const FTransform& Transform)
{
	// Map name.
	if (!bLocalMap)
	{
		MapName = UGameplayStatics::GetCurrentLevelName(GetWorld());
	}
	// Tile size
	TileSize = FMath::Pow(static_cast<float>(2), static_cast<float>(TilePower));
	// Texture size
	TextureScale = FMath::Pow(static_cast<float>(2), static_cast<float>(Power));
	// Square shape end point.
	EndPoint = FVector(EndPoint.X, EndPoint.X * UKismetMathLibrary::SignOfFloat(EndPoint.Y), 0);
	// Update camera location.
	Capture2D->SetWorldLocation(FVector(GetActorLocation().X + 0.5 * EndPoint.X, GetActorLocation().Y + 0.5 * EndPoint.Y, GetActorLocation().Z));
	// Set up render target
	if (!Capture2D->TextureTarget)
	{
		UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
		RenderTarget->InitAutoFormat(TileSize, TileSize);
		RenderTarget->ClearColor = FLinearColor::Black;
		RenderTarget->UpdateResourceImmediate(true);
		Capture2D->TextureTarget = RenderTarget;
	}
	else
	{
		Capture2D->TextureTarget->InitAutoFormat(TileSize, TileSize);
	}
}
