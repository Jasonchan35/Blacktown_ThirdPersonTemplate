#include "MyUIMainWidget.h"
#include "../MyPlayerController.h"

FVector2f UMyUIMainWidget::GetCrossHairPos()
{
	return UI_CrossHair->GetPaintSpaceGeometry().GetAbsolutePosition();
}

void UMyUIMainWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	auto* PC = GetMyPlayerController();
	if (!PC)
		return;

	auto* Ch = PC->GetMyCharacter();
	if (!Ch)
		return;

	auto Ability = Ch->GetCurrentAbility();
	bool ShowCrossHair = (Ability == EMyAbility::UltraHand);

	UI_CrossHair->SetVisibility(ShowCrossHair ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}
