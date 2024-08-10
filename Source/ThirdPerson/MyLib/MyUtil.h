#pragma once

#include "MyType.h"

#define MY_CLASS_FINDER(OUT_FIELD, ASSET_PATH)                             \
	do                                                                     \
	{                                                                      \
		using Class = MyType::RemoveRef<decltype(OUT_FIELD)>;              \
		static ConstructorHelpers::FClassFinder<Class> Finder(ASSET_PATH); \
		OUT_FIELD = Finder.Class;                                          \
	}                                                                      \
	while (false)

#define MY_OBJECT_FINDER(OUT_FIELD, ASSET_PATH)                             \
	do                                                                      \
	{                                                                       \
		using Class = MyType::RemoveRef<decltype(OUT_FIELD)>;               \
		static ConstructorHelpers::FObjectFinder<Class> Finder(ASSET_PATH); \
		OUT_FIELD = Finder.Object;                                          \
	}                                                                       \
	while (false)
