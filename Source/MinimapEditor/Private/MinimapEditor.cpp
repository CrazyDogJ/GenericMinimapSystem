#include "MinimapEditor.h"

//#include "SplineWidgetDetailsCustomization.h"
//#include "Widgets/SplineWidget.h"
#include "ISettingsModule.h"
#include "MinimapSettings.h"

#define LOCTEXT_NAMESPACE "FMinimapEditorModule"

void FMinimapEditorModule::StartupModule()
{
	// Settings register
	//FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	//PropertyModule.RegisterCustomClassLayout(USplineWidget::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FSplineWidgetDetailCustomization::MakeInstance));

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "Minimap", LOCTEXT("RuntimeSettingsName", "Minimap"), LOCTEXT("RuntimeSettingsDescription", "Configure minimap"), GetMutableDefault<UMinimapSettings>());
	}
}

void FMinimapEditorModule::ShutdownModule()
{
	if (!UObjectInitialized())
	{
		return;
	}

	// Settings unregister
	//FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	//PropertyModule.UnregisterCustomClassLayout(USplineWidget::StaticClass()->GetFName());

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "Minimap");
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FMinimapEditorModule, MinimapEditor)