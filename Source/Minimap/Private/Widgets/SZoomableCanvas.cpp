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

void SZoomableCanvas::SetEnableInput(bool bNewEnableInput)
{
	bEnableInput = bNewEnableInput;
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
	// SetClipping(EWidgetClipping::ClipToBounds);
	
	/*
	ChildSlot
	[
		// Populate the widget
	];
	*/
}

void SZoomableCanvas::AddTile(TSharedRef<SWidget> InWidget, FVector2D InPosition, FVector2D InSize)
{
	TileSlots.Add(FTileSlot(InWidget, InPosition, InSize));
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

FReply SZoomableCanvas::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bEnableInput)
	{
		return SPanel::OnMouseButtonDown(MyGeometry, MouseEvent);
	}
	
	// 使用右键或中键进行平移
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = true;
		LastMousePos = MouseEvent.GetScreenSpacePosition();
        
		// 捕获鼠标，确保鼠标移出控件范围后依然能接收事件
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}
	
	return FReply::Unhandled();
}

FReply SZoomableCanvas::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bEnableInput)
	{
		return SPanel::OnMouseButtonUp(MyGeometry, MouseEvent);
	}
	
	if (bIsDragging)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	
	return FReply::Unhandled();
}

FReply SZoomableCanvas::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bEnableInput)
	{
		return SPanel::OnMouseMove(MyGeometry, MouseEvent);
	}
	
	if (bIsDragging)
	{
		FVector2D CursorDelta = MouseEvent.GetScreenSpacePosition() - LastMousePos;
		ViewOffset += CursorDelta; // 移动视图偏移
		LastMousePos = MouseEvent.GetScreenSpacePosition();
		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

FReply SZoomableCanvas::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bEnableInput)
	{
		return SPanel::OnMouseWheel(MyGeometry, MouseEvent);
	}
	
	float OldScale = ViewScale;
	ViewScale = FMath::Clamp(ViewScale + MouseEvent.GetWheelDelta() * ZoomStep, ScaleMin, ScaleMax);
 
	// 获取鼠标在控件内的局部位置
	FVector2D MouseLocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
 
	// 缩放补偿公式：调整 Offset 以实现对准鼠标缩放
	ViewOffset = MouseLocalPos - (MouseLocalPos - ViewOffset) * (ViewScale / OldScale);
 
	return FReply::Handled();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
