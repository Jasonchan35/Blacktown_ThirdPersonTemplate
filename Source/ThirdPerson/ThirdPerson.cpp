// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThirdPerson.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

#if WITH_EDITOR
	#include "Interfaces/IMainFrameModule.h"
#endif // WITH_EDITOR

#define LOCTEXT_NAMESPACE "ThirdPerson_Module"

class ThirdPerson_Module : public IModuleInterface
{
	using ThisClass = ThirdPerson_Module;

public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
#if WITH_EDITOR
	void OnEditorStartup();
	void OnEditorMainFrameStartup();
#endif
};

inline void ThirdPerson_Module::StartupModule()
{
#if WITH_EDITOR
	OnEditorStartup();
#endif
}

#if WITH_EDITOR
void ThirdPerson_Module::OnEditorStartup()
{
	IMainFrameModule& MainFrameModule = IMainFrameModule::Get();
	if (MainFrameModule.IsWindowInitialized())
	{
		OnEditorMainFrameStartup();
	}
	else
	{
		MainFrameModule.OnMainFrameCreationFinished().AddLambda(
			[this](TSharedPtr<SWindow> InRootWindow, bool bIsRunningStartupDialog) {
				OnEditorMainFrameStartup();
			});
	}
}

void ThirdPerson_Module::OnEditorMainFrameStartup()
{
	MyConsole::SetVar(L"t.MaxFPS", 60); // limit the fps to avoid burning my GPU in editor during development
	MyConsole::EditorCmd(L"stat fps"); // show fps
}
#endif

void ThirdPerson_Module::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

// IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, ThirdPerson, "ThirdPerson" );
IMPLEMENT_PRIMARY_GAME_MODULE(ThirdPerson_Module, ThirdPerson, "ThirdPerson");
