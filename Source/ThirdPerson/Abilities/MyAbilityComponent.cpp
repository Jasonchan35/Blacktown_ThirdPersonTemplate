#include "MyAbilityComponent.h"
#include "../MyPlayerController.h"

AMyCharacter* UMyAbilityComponent::GetMyCharacter()
{
	return GetOwner<AMyCharacter>();
}

AMyPlayerController* UMyAbilityComponent::GetMyPlayerController()
{
	auto* Ch = GetOwner<ACharacter>();
	return Ch ? Ch->GetController<AMyPlayerController>() : nullptr;
}
