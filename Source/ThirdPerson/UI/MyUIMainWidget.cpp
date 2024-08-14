#include "MyUIMainWidget.h"
#include "../MyPlayerController.h"
#include "../Abilities/MyUltraHandComponent.h"
#include "../Abilities/MyFusedComponent.h"

FVector2f UMyUIMainWidget::GetCrossHairPos()
{
	return UI_CrossHair_Widget->GetPaintSpaceGeometry().GetAbsolutePosition();
}

void UMyUIMainWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	bShow_CrossHair_Widget		= false;
	bShow_Confirm_Widget		= false;
	bShow_Cancel_Widget			= false;
	bShow_ForwardBack_Widget	= false;
	bShow_Deattach_Widget		= false;

	if (auto* Ch = GetMyCharacter())
	{
		if (Ch->GetVelocity().Length() > 100)
			bShow_WASD_Widget = false; // hide after first character move

		auto* Ability = Ch->GetCurrentAbilityComponent();
		if (auto* UltraHand = Cast<UMyUltraHandComponent>(Ability))
			TickUltraHandMode(UltraHand);
	}

	auto ShowWidget = [](UWidget* W, bool bShow)
	{
		if (!W)
			return;

		auto Value = bShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
		if (Value == W->GetVisibility())
			return;

		W->SetVisibility(Value);
	};

	#define OP(Widget) do { ShowWidget(UI_##Widget, bShow_##Widget); } while(false)
	//----
		OP(WASD_Widget			);
		OP(CrossHair_Widget		);
		OP(Confirm_Widget		);
		OP(Cancel_Widget		);
		OP(ForwardBack_Widget	);
		OP(Deattach_Widget		);
	#undef OP
}

void UMyUIMainWidget::TickUltraHandMode(UMyUltraHandComponent* UltraHand)
{
	switch (UltraHand->GetMode())
	{
		case EMyUltraHandMode::SearchTarget:
		{
			bShow_CrossHair_Widget = true;

			if (UltraHand->GetTargetActor())
			{
				bShow_Confirm_Widget = true;
				UI_Confirm_Text->SetText(Text_Grab);
			}
		}
		break;

		case EMyUltraHandMode::HoldTarget:
		{
			bShow_Cancel_Widget = true;
			bShow_ForwardBack_Widget = true;

			if (auto* Target = UltraHand->GetTargetActor())
			{
				if (auto* Group = MyFuseHelper::FindGroup(Target))
				{
					bShow_Deattach_Widget = true;
				}
			}

			if (UltraHand->HasFusable())
			{
				bShow_Confirm_Widget = true;
				UI_Confirm_Text->SetText(Text_Attach);
			}
		}
		break;
	}
}
