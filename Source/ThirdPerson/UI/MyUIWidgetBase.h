#pragma once

#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListentry.h"

#include "MyUIWidgetBase.generated.h"

class AMyPlayerController;
class AMyCharacter;

UCLASS(Abstract)
class UMyUIWidgetBase : public UUserWidget
{
	GENERATED_BODY()
public:
	void SafeAddToViewport(int ZOrder = 0);
	void SafeRemoveFromParent();

	AMyPlayerController*	GetMyPlayerController();
	AMyCharacter*			GetMyCharacter();

private:
	// Disable Super functions
	void AddToViewport(int ZOrder = 0) { Super::AddToViewport(); }
	void RemoveFromParent() { Super::RemoveFromParent(); }
};
