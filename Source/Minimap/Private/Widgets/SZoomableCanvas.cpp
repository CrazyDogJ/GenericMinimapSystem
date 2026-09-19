// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/SZoomableCanvas.h"

#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

SZoomableCanvas::SZoomableCanvas()
	: Children(this)
{
}

void SZoomableCanvas::SetDebugScreenString(bool bNewDebugScreenString)
{
	bDebugScreenString = bNewDebugScreenString;
}

void SZoomableCanvas::SetZoomStep(float InZoomStep)
{
	ZoomStep = InZoomStep;
}

float SZoomableCanvas::GetScale() const
{
	return ViewScale;
}

void SZoomableCanvas::SetMinScale(float InScaleMin)
{
	ScaleMin = InScaleMin;
	ViewScale = FMath::Clamp(ViewScale, ScaleMin, ScaleMax);
}

void SZoomableCanvas::SetMaxScale(float InScaleMax)
{
	ScaleMax = InScaleMax;
	ViewScale = FMath::Clamp(ViewScale, ScaleMin, ScaleMax);
}

void SZoomableCanvas::SetViewScale(float InViewScale)
{
	ViewScale = InViewScale;
}

void SZoomableCanvas::SetViewOffset(FVector2D InViewOffset)
{
	ViewOffset = InViewOffset;
}

void SZoomableCanvas::Construct(const FArguments& InArgs)
{
}

void SZoomableCanvas::AddTile(const TSharedRef<SWidget> InWidget, const FVector2D InPosition, const FVector2D InSize)
{
	TileSlots.Add(FTileSlot(InWidget, InPosition, InSize));
	Children.Add(InWidget);
}

void SZoomableCanvas::RemoveTile(const TSharedRef<SWidget> InWidget)
{
	TileSlots.Remove(FTileSlot(InWidget));
	Children.Remove(InWidget);
}

void SZoomableCanvas::ClearTiles()
{
	TileSlots.Empty();
	Children.Empty();
}

void SZoomableCanvas::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
{
	const FVector2D PanelSize = AllottedGeometry.GetLocalSize();
	const FSlateRect VisibleRect(0, 0, PanelSize.X, PanelSize.Y);
	
#if !UE_BUILD_SHIPPING
	int32 VisibleCount = 0;
#endif
	
	for (const FTileSlot& Slot : TileSlots)
	{
		// 将 Canvas 坐标转换为当前视图的 Local 坐标
		FVector2D ChildLocalPos = (Slot.Position * ViewScale) + ViewOffset;
		FVector2D ChildLocalSize = Slot.Size * ViewScale;
 
		FSlateRect ChildRect(ChildLocalPos, ChildLocalPos + ChildLocalSize);
 
		// 仅当 Tile 与当前视图区域相交时才添加（虚拟化）
		if (FSlateRect::DoRectanglesIntersect(VisibleRect, ChildRect))
		{
			VisibleCount++;
			ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(
				Slot.Widget,
				ChildLocalPos,
				ChildLocalSize
			));
		}
	}
	
#if !UE_BUILD_SHIPPING
	if (bDebugScreenString)
	{
		FString DebugMsg = FString::Printf(TEXT("Total: %d | Visible: %d"), TileSlots.Num(), VisibleCount);
		GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Green, DebugMsg);
	}
#endif
}

FVector2D SZoomableCanvas::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	FBox2D CombinedBox(ForceInit);
	for (const FTileSlot& Slot : TileSlots)
	{
		FVector2D TopLeft = Slot.Position;
		FVector2D BottomRight = Slot.Position + Slot.Size;
 
		CombinedBox += TopLeft;
		CombinedBox += BottomRight;
	}
	
	return CombinedBox.GetSize();
}

int32 SZoomableCanvas::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                               const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
                               const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	this->OnArrangeChildren(AllottedGeometry, ArrangedChildren);
	
	int32 RetLayerId = LayerId;
	PaintArrangedChildren(Args, ArrangedChildren, AllottedGeometry, MyCullingRect, OutDrawElements, RetLayerId++, InWidgetStyle, bParentEnabled);
	
	return RetLayerId;
}

FChildren* SZoomableCanvas::GetChildren()
{
	return &Children;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
