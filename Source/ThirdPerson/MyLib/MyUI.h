#pragma once

#include "MyLog.h"

struct MyUI
{
	MyUI() = delete;

	template <class T, class OWNER_T>
	static T* CreateWidget(OWNER_T* Owner, TSubclassOf<T> Class, bool bAddToViewport = true, int32 ZOrder = 0);
};

template <class T, class OWNER_T>
inline T* MyUI::CreateWidget(OWNER_T* Owner, TSubclassOf<T> Class, bool bAddToViewport, int32 ZOrder)
{
	static_assert(std::is_base_of<UWidget, T>::value);

	if (!Class)
	{
		MY_LOG_ERROR(L"CreateWidget Class ({}) is null", T::StaticClass()->GetFName());
		return nullptr;
	}

	auto* Widget = ::CreateWidget<T>(Owner, Class);
	if (!Widget)
	{
		MY_LOG_ERROR(L"CreateWidget() Error", T::StaticClass()->GetFName());
		return nullptr;
	}

	if (bAddToViewport)
		Widget->SafeAddToViewport(ZOrder);

	return Widget;
}
