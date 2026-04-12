// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SMapGridPanel.h"
#include "Components/UniformGridPanel.h"
#include "MapGridPanel.generated.h"

/**
 * 
 */
UCLASS()
class MINIMAP_API UMapGridPanel : public UPanelWidget
{
	GENERATED_BODY()

public:

	UMapGridPanel(const FObjectInitializer& ObjectInitializer);
	
	UFUNCTION(BlueprintCallable)
	void SetCoordinate(FIntPoint InCoordinate);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter, Setter, BlueprintSetter="SetAxisCount", Category="Child Layout")
	int32 AxisCount;

	UFUNCTION(BlueprintPure)
	int32 GetAxisCount() const {return AxisCount;}
	
	UFUNCTION(BlueprintCallable, Category="Child Layout")
	void SetAxisCount(int32 InAxisCount);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter, Setter, BlueprintSetter="SetTextures", Category="Child Layout")
	TArray<UTexture2D*> Textures;

	UFUNCTION(BlueprintPure)
	TArray<UTexture2D*> GetTextures() const {return Textures;}
	
	UFUNCTION(BlueprintCallable, Category="Child Layout")
	void SetTextures(TArray<UTexture2D*> InTextures);
	
	//~ UWidget interface
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	//~ End of UWidget interface

protected:

	TSharedPtr<SMapGridPanel> MyMapGridPanel;
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
