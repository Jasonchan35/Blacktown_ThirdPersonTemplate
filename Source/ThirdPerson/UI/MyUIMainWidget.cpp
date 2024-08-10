#include "MyUIMainWidget.h"

FVector2f UMyUIMainWidget::GetCrossHairPos()
{
	return UI_CrossHair->GetPaintSpaceGeometry().GetAbsolutePosition();
}
