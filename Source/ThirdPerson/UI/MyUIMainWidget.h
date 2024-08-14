#pragma once

#include "MyUIWidgetBase.h"
#include "MyUIMainWidget.generated.h"

UCLASS(Abstract)
class UMyUIMainWidget : public UMyUIWidgetBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Transient, meta=(BindWidget))
	TObjectPtr<UWidget> UI_CrossHair_Widget;

	UPROPERTY(VisibleAnywhere, Transient, meta=(BindWidget))
	TObjectPtr<UWidget> UI_WASD_Widget;

	UPROPERTY(VisibleAnywhere, Transient, meta=(BindWidget))
	TObjectPtr<UWidget> UI_Confirm_Widget;

	UPROPERTY(VisibleAnywhere, Transient, meta=(BindWidget))
	TObjectPtr<UTextBlock> UI_Confirm_Text;

	UPROPERTY(VisibleAnywhere, Transient, meta=(BindWidget))
	TObjectPtr<UWidget> UI_Cancel_Widget;

	UPROPERTY(VisibleAnywhere, Transient, meta=(BindWidget))
	TObjectPtr<UWidget> UI_Deattach_Widget;

	UPROPERTY(VisibleAnywhere, Transient, meta=(BindWidget))
	TObjectPtr<UWidget> UI_ForwardBack_Widget;

	UPROPERTY(EditAnywhere)	FText	Text_Grab;
	UPROPERTY(EditAnywhere)	FText	Text_Attach;

public:
	FVector2f	GetCrossHairPos();

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void TickUltraHandMode(class UMyUltraHandComponent* UltraHand);

	bool bShow_WASD_Widget = true;

	bool bShow_CrossHair_Widget		= false;
	bool bShow_Confirm_Widget		= false;
	bool bShow_Cancel_Widget		= false;
	bool bShow_ForwardBack_Widget	= false;
	bool bShow_Deattach_Widget		= false;
};