#pragma once

#include "UObject/ConstructorHelpers.h"

template<class T> struct MY_CDO_FINDER_Helper;

template<class T> struct MY_CDO_FINDER_Helper< TSubclassOf<T> >
: public ConstructorHelpers::FClassFinder<T>
{
	using Super = ConstructorHelpers::FClassFinder<T>;

	MY_CDO_FINDER_Helper(const TCHAR* AssetPath) : Super(AssetPath) {}
	TSubclassOf<T> 	GetResult() { return Super::Class; }
};

template<class T> struct MY_CDO_FINDER_Helper< TObjectPtr<T> >
: public ConstructorHelpers::FObjectFinder<T>
{
	using Super = ConstructorHelpers::FObjectFinder<T>;

	MY_CDO_FINDER_Helper(const TCHAR* AssetPath) : Super(AssetPath) {}
	TObjectPtr<T> 	GetResult() { return Super::Object; }
};

#define MY_CDO_FINDER(OUT_FIELD, ASSET_PATH)            \
	do                                                  \
	{                                                   \
		static MY_CDO_FINDER_Helper<decltype(OUT_FIELD)> Finder(ASSET_PATH); \
		OUT_FIELD = Finder.GetResult();  \
	}                                                   \
	while (false)
//----
