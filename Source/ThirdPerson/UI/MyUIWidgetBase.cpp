#include "MyUIWidgetBase.h"
#include "../MyPlayerController.h"

void UMyUIWidgetBase::SafeAddToViewport(int ZOrder)
{
	if (!IsInViewport())
		AddToViewport(ZOrder);
}

void UMyUIWidgetBase::SafeRemoveFromParent()
{
	if (IsInViewport())
		RemoveFromParent();
}

AMyPlayerController* UMyUIWidgetBase::GetMyPlayerController()
{
	return GetOwningPlayer<AMyPlayerController>();
}

AMyCharacter* UMyUIWidgetBase::GetMyCharacter()
{
	auto* PC = GetMyPlayerController();
	return PC ? PC->GetMyCharacter() : nullptr;
}
