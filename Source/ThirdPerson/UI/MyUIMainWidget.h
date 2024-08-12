#pragma once

#include "MyUIWidgetBase.h"
#include "MyUIMainWidget.generated.h"

UCLASS(Abstract)
class UMyUIMainWidget : public UMyUIWidgetBase
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Transient, meta=(BindWidget))
	TObjectPtr<UImage>	UI_CrossHair;

public:
	FVector2f	GetCrossHairPos();

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};