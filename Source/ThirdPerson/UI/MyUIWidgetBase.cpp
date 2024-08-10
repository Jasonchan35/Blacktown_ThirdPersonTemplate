#include "MyUIWidgetBase.h"

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
