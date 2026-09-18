// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MinimapUserWidget.h"

#include "MinimapMapData.h"
#include "MinimapSubsystem.h"
#include "Widgets/MapPinUserWidget.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Widgets/MinimapWidgetInterface.h"

UMinimapUserWidget::UMinimapUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MarkerClassType = TYPE_MINIMAP;
}

void UMinimapUserWidget::NativeConstruct()
{
	InitializeRadius();
	InitializeMapPins();
	ManagerEvents(true);
	
	Super::NativeConstruct();
}

int32 UMinimapUserWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	UpdateViewAngle();
	UpdateLocalPlayerAngle();
	UpdateNorthWidgets();
	UpdateMarkers();
	UpdateMinimapImageParameters();
	
	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle,
	                          bParentEnabled);
}

void UMinimapUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	UpdateInterpRadius(InDeltaTime);
	UpdateMarkersVisibilities();
	
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UMinimapUserWidget::NativeDestruct()
{
	ManagerEvents(false);
	
	Super::NativeDestruct();
}

bool UMinimapUserWidget::IsInRadius(const FVector& CenterLocation, const FVector& CheckLocation,
	const float& Radius)
{
	const FVector2D Center2D(CenterLocation);
	const FVector2D Check2D(CheckLocation);
	return FVector2D::Distance(Center2D, Check2D) <= Radius;
}

bool UMinimapUserWidget::IsInRadius(const FVector& CenterLocation, const FVector& CheckLocation) const
{
	return IsInRadius(CenterLocation, CheckLocation, InterpRadius);
}

bool UMinimapUserWidget::IsInRadius(const FVector& CenterLocation, const FGuid& InMapPinId) const
{
	if (const auto Interface = TryGetDataInterface())
	{
		FVector OutCheckLocation;
		Interface->GetLocation(InMapPinId, OutCheckLocation);
		return IsInRadius(CenterLocation, OutCheckLocation, InterpRadius);
	}

	// Default is not in radius if we can't find the check location.
	return false;
}

bool UMinimapUserWidget::IsInRadius(const FGuid& InMapPinId) const
{
	if (const auto LocalPlayerActor = GetLocalPlayerActor())
	{
		return IsInRadius(LocalPlayerActor->GetActorLocation(), InMapPinId);
	}
	
	// Default is not in radius if we can't find the check location.
	return false;
}

void UMinimapUserWidget::InitializeRadius()
{
	InterpRadius = DefaultRadius;
	TargetRadius = DefaultRadius;
}

void UMinimapUserWidget::InitializeMapPins()
{
	if (const auto Interface = TryGetDataInterface())
	{
		TSet<FGuid> MapPins;
		if (Interface->GetRegisteredMapPins(MapPins))
		{
			for (const auto Itr : MapPins)
			{
				OnMapPinAdd(Itr);
			}
		}
	}
}

void UMinimapUserWidget::ManagerEvents(const bool& BindOrUnbind)
{
	if (const auto Interface = TryGetDataInterface())
	{
		if (BindOrUnbind)
		{
			if (const auto AddEvents = Interface->GetMapPinAddEvent())
			{
				AddEvents->AddUObject(this, &ThisClass::OnMapPinAdd);
			}
			if (const auto RemoveEvents = Interface->GetMapPinRemoveEvent())
			{
				RemoveEvents->AddUObject(this, &ThisClass::OnMapPinRemove);
			}
		}
		else
		{
			if (const auto AddEvents = Interface->GetMapPinAddEvent())
			{
				AddEvents->RemoveAll(this);
			}
			if (const auto RemoveEvents = Interface->GetMapPinRemoveEvent())
			{
				RemoveEvents->RemoveAll(this);
			}
		}
	}
}

void UMinimapUserWidget::OnMapPinAdd(const FGuid& Guid)
{
	if (const auto Interface = TryGetDataInterface())
	{
		if (IsInRadius(Guid) || Interface->GetIsAlwaysOnMinimap(Guid))
		{
			// We don't add local marker to the minimap.
			if (!IsLocalPlayerMarker(Guid))
			{
				AddMapPin(Guid);
			}
		}
	}
}

void UMinimapUserWidget::OnMapPinRemove(const FGuid& Guid)
{
	RemoveMapPin(Guid);
}

void UMinimapUserWidget::UpdateInterpRadius(const float DeltaTime)
{
	if (const auto CurrentMapData = GetCurrentMapData())
	{
		InterpRadius = FMath::FInterpTo(InterpRadius, TargetRadius, DeltaTime, InterpSpeed);
		ZoomMultiplier = CurrentMapData->MapSize / InterpRadius;
	}
}

void UMinimapUserWidget::UpdateViewAngle() const
{
	if (GetViewAreaWidget())
	{
		GetViewAreaWidget()->SetRenderTransformAngle(bLockNorth ? GetViewAngle() : 0.0f);
	}
}

void UMinimapUserWidget::UpdateLocalPlayerAngle() const
{
	if (GetLocalPlayerWidget())
	{
		const float PlayerYaw = GetLocalPlayerTransform().Rotator().Yaw;
		const float InvertViewYaw = GetViewAngle() * -1;
		GetLocalPlayerWidget()->SetRenderTransformAngle(bLockNorth ? PlayerYaw : PlayerYaw + InvertViewYaw);
	}
}

void UMinimapUserWidget::UpdateNorthWidgets() const
{
	if (GetNorthPivotWidget())
	{
		GetNorthPivotWidget()->SetRenderTransformAngle(bLockNorth ? NorthYawOffset : (NorthYawOffset + GetViewAngle()) * -1);
	}
	
	if (GetNorthWidget())
	{
		GetNorthWidget()->SetRenderTransformAngle(NorthYawOffset * -1 + (bLockNorth ? 0 : GetViewAngle()));
	}
}

void UMinimapUserWidget::UpdateMarker(const IMinimapWidgetInterface* Interface, const FGuid& Id, UMapPinUserWidget* Widget) const
{
	FGameplayTag CategoryTag;
	const auto HasTag = Interface->GetCategoryTag(Id, CategoryTag);
	FVector Location;
	const auto HasLocation = Interface->GetLocation(Id, Location);
	float Yaw;
	const bool bHasYaw = Interface->GetYaw(Id, Yaw);
	if (HasTag && HasLocation)
	{
		UpdateMarker(Widget, CategoryTag, FVector2D(Location), Yaw, bHasYaw);
	}
}

void UMinimapUserWidget::UpdateMarker(UMapPinUserWidget* MapPin, const FGameplayTag CategoryTag, const FVector2D WorldPosition2D,
                                      const float Angle, const bool bRotate) const
{
	if (MapPin)
	{
		if (ShouldHide(CategoryTag))
		{
			MapPin->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			MapPin->SetVisibility(ESlateVisibility::Visible);
		}
		
		const auto WidgetPosition = GetWidgetPosition(WorldPosition2D);
		MapPin->SetRenderTranslation(WidgetPosition);
		MapPin->SetRenderTransformAngle(bRotate ? Angle : bLockNorth ? 0.0f : GetViewAngle());
	}
}

void UMinimapUserWidget::UpdateMarkers() const
{
	if (GetMarkersOverlay())
	{
		GetMarkersOverlay()->SetRenderTransformAngle(bLockNorth ? 0.0f : GetViewAngle() * -1.0);
	}

	if (const auto Interface = TryGetDataInterface())
	{
		const auto Copy = Markers;
		for (const auto Itr : Copy)
		{
			UpdateMarker(Interface, Itr.Key, Itr.Value);
		}
	}
}

void UMinimapUserWidget::UpdateMinimapImageParameters() const
{
	if (GetMinimapImageWidget())
	{
		GetMinimapImageWidget()->SetRenderTransformAngle(bLockNorth ? 0.0f : GetViewAngle() * -1.0);

		if (const auto ImageWidget = Cast<UImage>(GetMinimapImageWidget()))
		{
			if (const auto MatDyn = ImageWidget->GetDynamicMaterial(); MatDyn && GetCurrentMapData())
			{
				const auto DeltaPosition = GetLocalPlayerTransform().GetLocation() - GetCurrentMapData()->CaptureActorLocation;
				const float X = DeltaPosition.X / -GetCurrentMapData()->MapSize + 0.5;
				const float Y = DeltaPosition.Y / GetCurrentMapData()->MapSize - 0.5;
				MatDyn->SetVectorParameterValue("PlayerLocation", FVector(Y, X, 0.0f));
				MatDyn->SetScalarParameterValue("Zoom", ZoomMultiplier);
			}
		}
	}
}

void UMinimapUserWidget::UpdateMarkersVisibilities()
{
	const auto Interface = TryGetDataInterface();
	if (Interface)
	{
		if (LocalPawn)
		{
			TSet<FGuid> OutGuid;
			Interface->QueryHotPoints(LocalPawn->GetActorLocation(), InterpRadius, OutGuid);
			for (const auto Id : OutGuid)
			{
				if (IsInRadius(Id))
				{
					AddMapPin(Id);
				}
			}
		}
		
		// Check should add map pin.
		TSet<FGuid> Pins;
		if (Interface->GetRegisteredMapPins(Pins))
		{
			for (const auto Pin : Pins)
			{
				if (IsInRadius(Pin) && !IsLocalPlayerMarker(Pin))
				{
					AddMapPin(Pin);
				}
				else if (!Interface->GetIsAlwaysOnMinimap(Pin))
				{
					RemoveMapPin(Pin);
				}
			}
		}
	}
}

float UMinimapUserWidget::GetMinimapDisplayRadius_Implementation()
{
	// Default radius
	if (const auto CurrentMapData = GetCurrentMapData())
	{
		if (CurrentMapData->MapSize < DefaultRadius)
		{
			return CurrentMapData->MapSize;
		}
	}
	
	return DefaultRadius;
}

float UMinimapUserWidget::GetViewAngle_Implementation() const
{
	return GetOwningPlayer()->GetControlRotation().Yaw;
}

FTransform UMinimapUserWidget::GetLocalPlayerTransform_Implementation() const
{
	if (const auto LocalPlayerActor = GetLocalPlayerActor())
	{
		return LocalPlayerActor->GetTransform();
	}

	return FTransform::Identity;
}

float UMinimapUserWidget::GetDesiredRadius_Implementation() const
{
	if (GetMinimapImageWidget())
	{
		return GetMinimapImageWidget()->GetDesiredSize().X / 2;
	}

	return GetDesiredSize().X / 2;
}

UMinimapMapData* UMinimapUserWidget::GetCurrentMapData() const
{
	if (const auto MinimapSubsystem = GetMinimapSubsystem())
	{
		if (MinimapSubsystem->LocalMinimapMapData)
		{
			return MinimapSubsystem->LocalMinimapMapData;
		}
		
		return MinimapSubsystem->GetCurrentMinimapMapData();
	}

	return nullptr;
}

FVector2D UMinimapUserWidget::GetWidgetPosition(const FVector2D InWorldPosition2D) const
{
	const auto LocalPlayer2D = FVector2D(GetLocalPlayerTransform().GetLocation());
	const FVector2D TempPosition = (LocalPlayer2D - InWorldPosition2D) / InterpRadius * GetDesiredRadius();
	FVector2D Direction;
	float Length;
	TempPosition.ToDirectionAndLength(Direction, Length);
	Length = FMath::Clamp(Length, 0.0f, GetDesiredRadius());
	return FVector2D((Direction * Length).Y * -1, (Direction * Length).X);
}

bool UMinimapUserWidget::IsLocalPlayerMarker(const FGuid Guid) const
{
	if (const auto Interface = TryGetDataInterface())
	{
		return Interface->IsLocalPlayer(LocalPawn, Guid);
	}

	return false;
}

bool UMinimapUserWidget::IsCurrentMapDataValid() const
{
	return IsValid(GetCurrentMapData());
}
