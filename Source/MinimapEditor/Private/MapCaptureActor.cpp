// Fill out your copyright notice in the Description page of Project Settings.


#include "MapCaptureActor.h"

#include "EditorAssetLibrary.h"
#include "MapHotPointActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "MinimapSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetMathLibrary.h"
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
}

void AMapCaptureActor::CaptureMap()
{
#if WITH_EDITOR
	Capture2D->OrthoWidth = EndPoint.X;
	RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), TextureScale, TextureScale);
	Capture2D->TextureTarget = RenderTarget;
	Capture2D->CaptureScene();
	if (!bLocalMap)
	{
		MapName = UGameplayStatics::GetCurrentLevelName(GetWorld());
	}
	
	FString texName = FString(TEXT("T_")) + MapName;
	UMinimapSettings* Settings = GetMutableDefault<UMinimapSettings>();
	FString TotalFileName = FPaths::Combine(Settings->MapTexturePath, texName);
	
	FAssetRegistryModule* const AssetRegistryModule = FModuleManager::Get().GetModulePtr<FAssetRegistryModule>("AssetRegistry");
	if (!AssetRegistryModule)
	{
		return;
	}
	const IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
	TArray<FAssetData> outData;
	AssetRegistry.GetAssetsByPath(FName(*Settings->MapTexturePath), outData);
	for (auto data : outData)
	{
		if (data.AssetName.ToString() == texName)
		{
			UEditorAssetLibrary::DeleteAsset(TotalFileName);
		}
	}
	
	UTexture2D* tex = UKismetRenderingLibrary::RenderTargetCreateStaticTexture2DEditorOnly(RenderTarget, TotalFileName);
	
	if (auto Value = Settings->MapsInfos.Find(MapName))
	{
		if (UMinimapMapData* MapData = Value->LoadSynchronous())
		{
			WriteMapInfo(MapData, tex);
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
			WriteMapInfo(NewMapInfo, tex);
		}
		// save mapper class
		FString const PackageName = Package->GetName();
		FString const PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs);
		
		TSoftObjectPtr<UMinimapMapData> SoftRef(AssetPath + "." + AssetName);
		if (!bLocalMap)
		{
			Settings->MapsInfos.Add(UGameplayStatics::GetCurrentLevelName(GetWorld()), SoftRef);
			Settings->SaveConfig(CPF_Config, *Settings->GetDefaultConfigFilename());
		}
	}
#endif  
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

void AMapCaptureActor::OnConstruction(const FTransform& Transform)
{
	EndPoint = FVector(EndPoint.X, EndPoint.X * UKismetMathLibrary::SignOfFloat(EndPoint.Y), 0);
	Capture2D->SetWorldLocation(FVector(GetActorLocation().X + 0.5 * EndPoint.X, GetActorLocation().Y + 0.5 * EndPoint.Y, GetActorLocation().Z));
}
