// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/MinimapPlayerComponent.h"

#include "MinimapBlueprintFunctionLibrary.h"
#include "MinimapSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "MinimapZoneGraphAStar.h"
#include "Actors/MapPinActor.h"
#include "Blueprint/UserWidget.h"
#include "Components/MinimapGlobal.h"
#include "GameFramework/GameStateBase.h"
#include "Widgets/MainMapUserWidget.h"
#include "Widgets/MinimapUserWidget.h"

UMinimapPlayerComponent::UMinimapPlayerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMinimapPlayerComponent::OnMinimapGlobalReadyEvent(UMinimapGlobal* MinimapGlobal)
{
	if (bPendingPinRestore) RestoreTempPin(MinimapGlobal);
	if (const auto MinimapWidget = GetOrCreateMinimapWidget(MinimapGlobal))
	{
		MinimapWidget->AddToViewport();
	}
}

void UMinimapPlayerComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// const auto OwningPlayerController = Cast<APlayerController>(GetOwner());
	// const auto InputSubsystem = OwningPlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	// if (const auto IMC = InputMappingContext.LoadSynchronous())
	// {
	// 	InputSubsystem->AddMappingContext(IMC, 0);
	// }

	if (const auto Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>())
	{
		if (const auto Global = Sub->GetMinimapGlobal())
		{
			if (bPendingPinRestore) RestoreTempPin(Global);
			if (const auto MinimapWidget = GetOrCreateMinimapWidget(Global))
			{
				MinimapWidget->AddToViewport();
			}
		}
		else
		{
			Sub->OnMinimapGlobalReadyEvent.AddDynamic(this, &ThisClass::OnMinimapGlobalReadyEvent);
		}
	}
}

void UMinimapPlayerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const auto OwningPlayerController = Cast<APlayerController>(GetOwner());
	if (OwningPlayerController && OwningPlayerController->IsLocalPlayerController())
	{
		UpdateNavPath(DeltaTime);
	}
}

void UMinimapPlayerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 修复：控制器卸载时撤销全局通知并从视口移除它创建的界面。
	if (auto* Sub = UWorld::GetSubsystem<UMinimapSubsystem>(GetWorld()))
	{
		Sub->OnMinimapGlobalReadyEvent.RemoveDynamic(this, &ThisClass::OnMinimapGlobalReadyEvent);
	}
	if (IsValid(MinimapUserWidget)) MinimapUserWidget->RemoveFromParent();
	if (IsValid(MainMapUserWidget)) MainMapUserWidget->RemoveFromParent();
	MinimapUserWidget = nullptr;
	MainMapUserWidget = nullptr;
	Super::EndPlay(EndPlayReason);
	
	// const auto OwningPlayerController = Cast<APlayerController>(GetOwner());
	// if (const auto InputSubsystem = OwningPlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	// {
	// 	if (const auto IMC = InputMappingContext.LoadSynchronous())
	// 	{
	// 		InputSubsystem->RemoveMappingContext(IMC);
	// 	}
	// }
}

UMinimapUserWidget* UMinimapPlayerComponent::GetOrCreateMinimapWidget(UObject* DataSourceObject)
{
	if (MinimapUserWidget)
	{
		return MinimapUserWidget;
	}
	
	if (!DataSourceObject)
	{
		if (GetWorld() && GetWorld()->GetGameState())
		{
			DataSourceObject = GetWorld()->GetGameState()->GetComponentByClass<UMinimapGlobal>();
		}
	}
	
	const auto MinimapClass = UMinimapBlueprintFunctionLibrary::GetMinimapWidgetClass();
	const auto OwningPlayerController = Cast<APlayerController>(GetOwner());
	if (OwningPlayerController && OwningPlayerController->IsLocalPlayerController() && MinimapClass)
	{
		MinimapUserWidget = CreateWidget<UMinimapUserWidget, APlayerController*>(OwningPlayerController, MinimapClass);
		// 修复：玩家退出或控件类无效时，CreateWidget 可能失败。
		if (MinimapUserWidget) MinimapUserWidget->MinimapDataSourceObject = DataSourceObject;
	}

	return MinimapUserWidget;
}

UMainMapUserWidget* UMinimapPlayerComponent::GetOrCreateMainMapWidget(UObject* DataSourceObject)
{
	if (MainMapUserWidget)
	{
		return MainMapUserWidget;
	}

	if (!DataSourceObject)
	{
		if (GetWorld() && GetWorld()->GetGameState())
		{
			DataSourceObject = GetWorld()->GetGameState()->GetComponentByClass<UMinimapGlobal>();
		}
	}
	
	const auto MainMapClass = UMinimapBlueprintFunctionLibrary::GetMainmapWidgetClass();
	const auto OwningPlayerController = Cast<APlayerController>(GetOwner());
	if (OwningPlayerController && OwningPlayerController->IsLocalPlayerController() && MainMapClass)
	{
		MainMapUserWidget = CreateWidget<UMainMapUserWidget, APlayerController*>(OwningPlayerController, MainMapClass);
		// 修复：玩家退出或控件类无效时，CreateWidget 可能失败。
		if (MainMapUserWidget) MainMapUserWidget->MinimapDataSourceObject = DataSourceObject;
	}

	return MainMapUserWidget;
}

bool UMinimapPlayerComponent::ShouldShowNavPath() const
{
	return bShouldUpdateNavQuery && bPathPointsValid;
}

void UMinimapPlayerComponent::UpdateNavPath(const float& DeltaTime)
{
	const auto Sub = GetWorld()->GetSubsystem<UMinimapSubsystem>();
	if (!Sub)
	{
		return;
	}

	if (IsValid(Sub->LocalTempPinActor))
	{
		NavQueryEndPosition = Sub->LocalTempPinActor->GetActorLocation();
		if (!bShouldUpdateNavQuery) NavQueryTime = NavQueryPeriod;
		bShouldUpdateNavQuery = true;
	}
	else
	{
		bShouldUpdateNavQuery = false;
		NavQueryTime = 0.0f;
	}
	
	// Should update nav query.
	if (bShouldUpdateNavQuery)
	{
		// Update nav query start position using local player actor location.
		if (bAutoUpdateStartLocation)
		{
			if (const auto OwningPlayerController = Cast<APlayerController>(GetOwner()))
			{
				// 修复：重生或旁观时控制器可能没有 Pawn。
				const auto Pawn = OwningPlayerController->GetPawn();
				if (!IsValid(Pawn))
				{
					bPathPointsValid = false;
					return;
				}
				NavQueryStartPosition = Pawn->GetActorLocation();
			}
		}
		// Update nav query period.
		NavQueryTime += DeltaTime;
		if (NavQueryTime >= NavQueryPeriod)
		{
			NavQueryTime = 0.0f;
			// 修复：ZoneGraph 属于当前世界，查询与结果发布均在游戏线程按周期执行。
			FMinimapZoneGraphLanePath OutPath;
			NavQueryOutPathPoints.Reset();
			bPathPointsValid = UMinimapZoneGraphAStarLibrary::GetZoneGraphPathBP(this, NavQueryStartPosition, NavQueryEndPosition, NavQueryExtend, OutPath)
				&& UMinimapZoneGraphAStarLibrary::GetPathPoints(this, OutPath, NavQueryOutPathPoints);
		}
	}
}

void UMinimapPlayerComponent::OnRegister()
{
	Super::OnRegister();
	PersonalPoiStateList.WorldContextObject = this;
}

void UMinimapPlayerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ThisClass, PersonalPoiStateList, COND_OwnerOnly);
}

void UMinimapPlayerComponent::ServerPlaceTempPin_Implementation(FVector Location, const FSlateBrush& Brush)
{
	if (auto* Global = GetWorld()->GetSubsystem<UMinimapSubsystem>()->GetMinimapGlobal())
		Global->AddTempPin(Cast<APlayerController>(GetOwner()), Location, Brush);
}

void UMinimapPlayerComponent::ServerMoveTempPin_Implementation(FVector Location)
{
	if (auto* Global = GetWorld()->GetSubsystem<UMinimapSubsystem>()->GetMinimapGlobal())
		Global->ChangeTempPinLocation(Cast<APlayerController>(GetOwner()), Location);
}

void UMinimapPlayerComponent::ServerRemoveTempPin_Implementation()
{
	if (auto* Global = GetWorld()->GetSubsystem<UMinimapSubsystem>()->GetMinimapGlobal())
		Global->RemoveTempPin(Cast<APlayerController>(GetOwner()));
}

FMinimapPlayerSaveData UMinimapPlayerComponent::GetSaveData() const
{
	FMinimapPlayerSaveData Data;
	auto* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->HasAuthority()) return Data;
	Data.DiscoveredPOIs = PersonalPoiStateList.PoiStateMap;
	Data.TempPinsByLevel = SavedTempPins;
	const auto* Global = GetWorld()->GetSubsystem<UMinimapSubsystem>()->GetMinimapGlobal();
	if (Global && !bPendingPinRestore)
	{
		const FString Level = UGameplayStatics::GetCurrentLevelName(this);
		if (const auto* Pin = Global->TempMapPinMapping.FindRef(PC); IsValid(Pin))
		{
			auto& Saved = Data.TempPinsByLevel.FindOrAdd(Level);
			Saved.Location = Pin->GetActorLocation();
			Saved.Brush = Pin->PinSlateBrush;
		}
		else Data.TempPinsByLevel.Remove(Level);
	}
	return Data;
}

void UMinimapPlayerComponent::LoadGame(const FMinimapPlayerSaveData& Data)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	PersonalPoiStateList.ReplaceWith(Data.DiscoveredPOIs);
	SavedTempPins = Data.TempPinsByLevel;
	bPendingPinRestore = true;
	if (auto* Global = GetWorld()->GetSubsystem<UMinimapSubsystem>()->GetMinimapGlobal()) RestoreTempPin(Global);
}

void UMinimapPlayerComponent::RestoreTempPin(UMinimapGlobal* Global)
{
	if (!Global || !GetOwner()->HasAuthority()) return;
	auto* PC = CastChecked<APlayerController>(GetOwner());
	bPendingPinRestore = false;
	if (const auto* Pin = SavedTempPins.Find(UGameplayStatics::GetCurrentLevelName(this)))
		Global->AddTempPin(PC, Pin->Location, Pin->Brush);
	else Global->RemoveTempPin(PC);
}
