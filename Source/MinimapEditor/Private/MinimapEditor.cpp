#include "MinimapEditor.h"

#include "SplineWidgetDetailsCustomization.h"
#include "Widgets/SplineWidget.h"

#define LOCTEXT_NAMESPACE "FMinimapEditorModule"

void FMinimapEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(USplineWidget::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FSplineWidgetDetailCustomization::MakeInstance));
}

void FMinimapEditorModule::ShutdownModule()
{
	if (!UObjectInitialized())
	{
		return;
	}

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout(USplineWidget::StaticClass()->GetFName());
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FMinimapEditorModule, MinimapEditor)