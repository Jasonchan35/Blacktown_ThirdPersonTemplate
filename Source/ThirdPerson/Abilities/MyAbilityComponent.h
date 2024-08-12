#pragma once

#include "Components/ActorComponent.h"
#include "MyAbilityComponent.generated.h"

class AMyCharacter;
class AMyPlayerController;

UCLASS(Abstract)
class UMyAbilityComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	virtual void IA_Confirm_Started() {}
	virtual void IA_Cancel_Started() {}

	virtual void SetAbilityActive(bool Active) {}

	AMyCharacter*	GetMyCharacter();
	AMyPlayerController* GetMyPlayerController();
};


