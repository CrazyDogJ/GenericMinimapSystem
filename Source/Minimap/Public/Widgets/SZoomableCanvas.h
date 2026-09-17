// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class MINIMAP_API SZoomableCanvas : public SPanel
{
public:
	SLATE_BEGIN_ARGS(SZoomableCanvas) {}
	SLATE_END_ARGS()
	
	SZoomableCanvas();
	
#if !UE_BUILD_SHIPPING
	void SetDebugScreenString(bool bNewDebugScreenString);
#endif
	void SetZoomStep(float InZoomStep);
	float GetScale() const;
	void SetMinScale(float InScaleMin);
	void SetMaxScale(float InScaleMax);
	void SetEnableInput(bool bNewEnableInput);
	void SetViewScale(float InViewScale);
	void SetViewOffset(FVector2D InViewOffset);
	
	float DebugMultiplier = 1.0f;
	
	void Construct(const FArguments& InArgs);
 
	// 添加 Tile 的接口
	void AddTile(TSharedRef<SWidget> InWidget, FVector2D InPosition, FVector2D InSize);
 
	// SWidget 核心重写
	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FChildren* GetChildren() override;
 
	// 输入处理
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
 
private:
	struct FTileSlot {
		TSharedRef<SWidget> Widget;
		FVector2D Position;
		FVector2D Size;
	};
 
	TArray<FTileSlot> TileSlots;
	TSlotlessChildren<SWidget> Children; // 子控件集合
 
	bool bEnableInput = false;
	
#if !UE_BUILD_SHIPPING
	bool bDebugScreenString = false;
#endif
	float ZoomStep = 0.1f;
	float ScaleMin = 0.1f;
	float ScaleMax = 5.0f;
	
	FVector2D ViewOffset = FVector2D::ZeroVector;
	float ViewScale = 1.0f;
	bool bIsDragging = false;
	FVector2D LastMousePos;
};
