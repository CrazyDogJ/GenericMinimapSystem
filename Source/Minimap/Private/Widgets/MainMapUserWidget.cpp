// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MainMapUserWidget.h"

#include "Components/MinimapComponent_Player.h"
#include "MinimapSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Widgets/MapPinUserWidget.h"
#include "Widgets/MinimapUserWidget.h"

void UMainMapUserWidget::OnValueChanged(float Value)
{
	Scale = Value;
	UpdateText();
}

void UMainMapUserWidget::OnMapPinAddEvent(const FGuid& MapPinId)
{
	if (!Markers.Find(MapPinId))
	{
		AddMapPin(MapPinId);
	}
}

void UMainMapUserWidget::OnMapPinRemoveEvent(const FGuid& MapPinId)
{
	RemoveMapPin(MapPinId);
}

void UMainMapUserWidget::OnHotPointFound(const FString& LevelName, const FGuid& Guid)
{
	if (!Markers.Find(Guid))
	{
		AddMapPin(Guid);
	}
}

void UMainMapUserWidget::OnHotPointRemove(const FString& LevelName, const FGuid& Guid)
{
	RemoveMapPin(Guid);
}

void UMainMapUserWidget::UpdateText() const
{
	if (const auto TextBlock = GetScaleDisplayText())
	{
		TextBlock->SetText(GetDisplayScaleText());
	}
}

void UMainMapUserWidget::UpdateDragging()
{
	float X;
	float Y;
	UWidgetLayoutLibrary::GetMousePositionScaledByDPI(GetOwningPlayer(), X, Y);
	const FVector2D MousePosition(X, Y);

	const auto Root = GetMapRootWidget();
	if (bMouseDragging && Root)
	{
		Offset += (MousePosition - CachedMousePosition) / FMath::Square(Scale);
		// Clamp offset.
		const auto LocalSize = Root->GetCachedGeometry().GetLocalSize();
		Offset = FVector2D::Clamp(Offset, FVector2D(-LocalSize / 2), FVector2D(LocalSize / 2));
	}
	
	CachedMousePosition = MousePosition;
}

void UMainMapUserWidget::UpdateTransform() const
{
	const auto SquareScale = FMath::Square(Scale);
	if (const auto Root = GetMapRootWidget())
	{
		Root->SetRenderScale(FVector2D(SquareScale));
		Root->SetRenderTranslation(Offset);
		const auto LocalSize = Root->GetCachedGeometry().GetLocalSize();
		Root->SetRenderTransformPivot((LocalSize / 2 - Offset) / LocalSize);
	}
	
	if (const auto MarkersOverlay = GetMarkersOverlay())
	{
		MarkersOverlay->SetRenderScale(FVector2D(1) / FVector2D(SquareScale));
	}
}

void UMainMapUserWidget::UpdateMarkers()
{
	if (const auto LocalComp = GetLocalPlayerMinimapComponent())
	{
		for (const auto Marker : Markers)
		{
			FMapPinStateEntry OutEntry;
			if (Marker.Value->GetMapPinState(OutEntry))
			{
				Marker.Value->SetRenderTranslation(WorldToWidget(FVector2D(OutEntry.Location), Scale));
				Marker.Value->SetRenderTransformAngle(OutEntry.bHasYaw ? OutEntry.Yaw : 0.0f);
				if (LocalComp->HiddenCategoryTags.Find(OutEntry.CategoryTag))
				{
					Marker.Value->SetVisibility(ESlateVisibility::Hidden);
				}
				else
				{
					Marker.Value->SetVisibility(IsMapPinVisible(Marker.Key) ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
				}
			}
		}
	}
}

void UMainMapUserWidget::NativeConstruct()
{
	if (const auto Slider = GetSliderWidget())
	{
		Slider->SetMinValue(MinScale);
		Slider->SetMaxValue(MaxScale);
		Slider->SetValue(Scale);
	}

	UpdateText();
	
	if (const auto MapImage = GetImageWidget())
	{
		MaterialInstance = MapImage->GetDynamicMaterial();
		// TODO : Do function that switch to local map later
		if (const auto Data = GetCurrentGlobalMapData())
		{
			MaterialInstance->SetTextureParameterValue("Map", Data->MapTexture);
		}
	}

	if (const auto Subsystem = GetMinimapSubsystem())
	{
		// Hot points bottom
		for (const auto HotPoint : GetLocalPlayerMinimapComponent()->GetFoundHotPoints())
		{
			for (const auto Index : HotPoint.Value.Indices)
			{
				if (!Markers.Find(Index))
				{
					AddMapPin(Index);
				}
			}
		}
		
		const FMapPinStateList LocalList = Subsystem->GetLocalPinStateList();
		const FMapPinStateList GlobalList = Subsystem->GetGlobalPinStateList();
		TArray<FMapPinStateEntry> AllEntries;
		AllEntries.Append(LocalList.StateEntries);
		AllEntries.Append(GlobalList.StateEntries);
		
		for (const auto Entry : AllEntries)
		{
			if (!Markers.Find(Entry.Id))
			{
				AddMapPin(Entry.Id);
			}
		}
	}

	ManageEvents(true);
	SetFocus();

	Super::NativeConstruct();
}

void UMainMapUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	UpdateDragging();
	UpdateTransform();
	UpdateMarkers();

	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UMainMapUserWidget::NativeDestruct()
{
	ManageEvents(false);
	// Clear widgets on destruct.
	for (auto Itr : Markers)
	{
		Itr.Value->RemoveFromParent();
		WidgetPool.Release(Itr.Value);
	}
	Markers.Empty();

	Super::NativeDestruct();
}

TSubclassOf<UMapPinUserWidget> UMainMapUserWidget::GetCustomClass(const FGuid& Guid)
{
	if (const auto LocalComp = GetLocalPlayerMinimapComponent())
	{
		FMapPinStateEntry OutEntry;
		if (LocalComp->GetMinimapPinState(Guid, OutEntry))
		{
			if (OutEntry.CustomMainmapWidgetClass)
				return OutEntry.CustomMainmapWidgetClass;
		}
	}
	
	return Super::GetCustomClass(Guid);
}

FReply UMainMapUserWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(DragKey))
	{
		bMouseDragging = true;
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const auto MapImageWidget = GetImageWidget();
	if (InMouseEvent.IsMouseButtonDown(MarkKey) && MapImageWidget)
	{
		const auto ScreenPosition = InMouseEvent.GetScreenSpacePosition();
		const auto Local = MapImageWidget->GetCachedGeometry().AbsoluteToLocal(ScreenPosition);
		if (const auto LocalPlayerComp = GetLocalPlayerMinimapComponent())
		{
			LocalPlayerComp->AddTempPin_MainMap(
				FVector2D(GetCaptureCenter()) - WidgetToWorld(Local),
				GetCurrentGlobalMapData(), ECC_Visibility);
		}
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UMainMapUserWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == DragKey)
	{
		bMouseDragging = false;
	}
	
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UMainMapUserWidget::ManageEvents(bool bManage)
{
	if (const auto Slider = GetSliderWidget())
	{
		if (bManage)
		{
			Slider->OnValueChanged.AddDynamic(this, &ThisClass::OnValueChanged);
		}
		else
		{
			Slider->OnValueChanged.RemoveAll(this);
		}
	}
	
	if (const auto Subsystem = GetMinimapSubsystem())
	{
		if (bManage)
		{
			Subsystem->OnMapPinAddEvent.AddDynamic(this, &ThisClass::OnMapPinAddEvent);
			Subsystem->OnMapPinRemoveEvent.AddDynamic(this, &ThisClass::OnMapPinRemoveEvent);
			Subsystem->OnHotPointFoundEvent.AddDynamic(this, &ThisClass::OnHotPointFound);
			Subsystem->OnHotPointRemoveEvent.AddDynamic(this, &ThisClass::OnHotPointRemove);
		}
		else
		{
			Subsystem->OnHotPointFoundEvent.RemoveAll(this);
		}
	}
}

bool UMainMapUserWidget::IsMapPinVisible_Implementation(FGuid Guid)
{
	return true;
}

FText UMainMapUserWidget::GetDisplayScaleText_Implementation() const
{
	const auto ScaleString = FString::Printf(TEXT("%f"), Scale);
	return FText::FromString(ScaleString);
}

UMinimapMapData* UMainMapUserWidget::GetCurrentGlobalMapData() const
{
	if (GetMinimapSubsystem())
	{
		return GetMinimapSubsystem()->GetCurrentMinimapMapData();
	}
	
	return nullptr;
}

FVector UMainMapUserWidget::GetCaptureCenter() const
{
	if (const auto Data = GetCurrentGlobalMapData())
	{
		const float HalfSize = Data->MapSize / 2;
		return Data->CaptureActorLocation + FVector(HalfSize, HalfSize, 0.0f);
	}

	// Invalid map data right now.
	return FVector::ZeroVector;
}

FVector2D UMainMapUserWidget::WorldToWidget(const FVector2D InWorldPosition2D, const float& InScale) const
{
	const auto CurrentData = GetCurrentGlobalMapData();
	const auto MapImage = GetImageWidget();
	if (CurrentData && MapImage)
	{
		FVector2D Dir;
		float Length;
		(InWorldPosition2D - FVector2D(GetCaptureCenter())).ToDirectionAndLength(Dir, Length);
		const FVector2D FinalVector = Dir * (Length / CurrentData->MapSize * MapImage->GetDesiredSize().X * FMath::Square(InScale));
		return FVector2D(FinalVector.Y, -FinalVector.X);
	}

	return FVector2D();
}

FVector2D UMainMapUserWidget::WidgetToWorld(FVector2D InLocalVector2D) const
{
	const auto CurrentData = GetCurrentGlobalMapData();
	const auto MapImage = GetImageWidget();
	if (CurrentData && MapImage)
	{
		const auto FinalVector2D = (InLocalVector2D / MapImage->GetDesiredSize() - FVector2D(0.5f)) * CurrentData->MapSize;
		return FVector2D(FinalVector2D.Y, -FinalVector2D.X);
	}
	
	return FVector2D();
}

void UMainMapUserWidget::LocalPawnCenter()
{
	if (LocalPawn)
	{
		const auto WidgetOffset = WorldToWidget(FVector2D(LocalPawn->GetActorLocation()), 1.0f);
		Offset = -WidgetOffset;
	}
}
