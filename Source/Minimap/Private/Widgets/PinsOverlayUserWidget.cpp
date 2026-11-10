// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/PinsOverlayUserWidget.h"

#include "MinimapBlueprintFunctionLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Widgets/MapPinUserWidget.h"
#include "Widgets/MinimapWidgetInterface.h"

void UPinsOverlayUserWidget::OnMapPinAddEvent(const FGuid& Guid)
{
	if (const auto Interface = TryGetDataInterface())
	{
		AddMapPin(Guid);
	}
}

void UPinsOverlayUserWidget::OnMapPinRemoveEvent(const FGuid& Guid)
{
	RemoveMapPin(Guid);
}

void UPinsOverlayUserWidget::ManageEvents(const bool ManageEvents)
{
	if (const auto Interface = TryGetDataInterface())
	{
		if (ManageEvents)
		{
			// 修复：数据接口的事件可以为空。
			if (auto* Event = Interface->GetMapPinAddEvent()) Event->AddUObject(this, &ThisClass::OnMapPinAddEvent);
			// 修复：数据接口的事件可以为空。
			if (auto* Event = Interface->GetMapPinRemoveEvent()) Event->AddUObject(this, &ThisClass::OnMapPinRemoveEvent);
		}
		else
		{
			// 修复：数据接口的事件可以为空。
			if (auto* Event = Interface->GetMapPinAddEvent()) Event->RemoveAll(this);
			// 修复：数据接口的事件可以为空。
			if (auto* Event = Interface->GetMapPinRemoveEvent()) Event->RemoveAll(this);
		}
	}
}

void UPinsOverlayUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (const auto Interface = TryGetDataInterface())
	{
		TSet<FGuid> Ids;
		if (Interface->GetRegisteredMapPins(Ids))
		{
			for (const auto Id : Ids)
			{
				if (Interface->GetIsOnOverlay(Id))
				{
					AddMapPin(Id);
				}
			}
		}
	}
	
	ManageEvents(true);
}

void UPinsOverlayUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	auto Interface = TryGetDataInterface();
	if (!Interface)
	{
		return;
	}
	
	for (const auto Marker : Markers)
	{
		if (Interface->GetIsOnOverlay(Marker.Key))
		{
			Marker.Value->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Marker.Value->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

int32 UPinsOverlayUserWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                          const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
                                          const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	auto Interface = TryGetDataInterface();
	if (!Interface)
	{
		return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle,
							  bParentEnabled);
	}
	
	for (const auto Marker : Markers)
	{
		const auto OwningPlayer = GetOwningPlayer();
		FVector Location;
		if (Interface->GetLocation(Marker.Key, Location))
		{
			bool bTargetBehindCamera = false;
			FVector2D OutScreenLocation;
			if (!UMinimapBlueprintFunctionLibrary::ProjectWorldToScreenBidirectional(OwningPlayer, Location,
				OutScreenLocation, bTargetBehindCamera, true)) continue;
			FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
			double Zero = 0;
			const bool bRound = !FMath::IsWithinInclusive(OutScreenLocation.X, Zero, ViewportSize.X)
				|| !FMath::IsWithinInclusive(OutScreenLocation.Y, Zero, ViewportSize.Y)
				|| bTargetBehindCamera;

			if (bRound)
			{
				FVector2D HalfViewportSize = ViewportSize / 2;
				FVector2D Direction;
				float Length;
				(OutScreenLocation - HalfViewportSize).ToDirectionAndLength(Direction, Length);
				FVector2D DirectionA;
				float LengthA;
				HalfViewportSize.ToDirectionAndLength(DirectionA, LengthA);
				const auto FinalScreenPos = HalfViewportSize + Direction * LengthA * 0.4 * RadiusMultiplier;
				FVector2D OutViewportPos;
				USlateBlueprintLibrary::ScreenToViewport(this, FinalScreenPos, OutViewportPos);
				Marker.Value->SetRenderTranslation(OutViewportPos);
				
				if (const auto ArrowWidget = GetPinArrowWidget(Marker.Value))
				{
					ArrowWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
					ArrowWidget->SetRenderTransformAngle(FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X)) + 90);
				}
			}
			else
			{
				FVector2D OutViewportPos;
				USlateBlueprintLibrary::ScreenToViewport(this, OutScreenLocation, OutViewportPos);
				Marker.Value->SetRenderTranslation(OutViewportPos);
				
				if (const auto ArrowWidget = GetPinArrowWidget(Marker.Value))
				{
					ArrowWidget->SetVisibility(ESlateVisibility::Hidden);
				}
			}
		}
	}
	
	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle,
	                          bParentEnabled);
}

void UPinsOverlayUserWidget::NativeDestruct()
{
	ManageEvents(false);
	
	Super::NativeDestruct();
}

TSubclassOf<UMapPinUserWidget> UPinsOverlayUserWidget::GetCustomClass(const FGuid& Guid)
{
	return MapPinUserWidgetClass;
}
