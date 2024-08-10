#pragma once

template<class T> struct MyType_RemoveRef;
template<class T> struct MyType_RemoveRef<TObjectPtr<T>		> { using Type = T; };
template<class T> struct MyType_RemoveRef<TSoftObjectPtr<T>	> { using Type = T; };
template<class T> struct MyType_RemoveRef<TSubclassOf<T>	> { using Type = T; };
template<class T> struct MyType_RemoveRef<T*				> { using Type = T; };

#define MY_CLASS_FINDER(OUT_FIELD, ASSET_PATH)                             \
	do                                                                     \
	{                                                                      \
		using Class = MyType_RemoveRef<decltype(OUT_FIELD)>::Type;         \
		static ConstructorHelpers::FClassFinder<Class> Finder(ASSET_PATH); \
		OUT_FIELD = Finder.Class;                                          \
	}                                                                      \
	while (false)

#define MY_OBJECT_FINDER(OUT_FIELD, ASSET_PATH)                             \
	do                                                                      \
	{                                                                       \
		using Class = MyType_RemoveRef<decltype(OUT_FIELD)>::Type;          \
		static ConstructorHelpers::FObjectFinder<Class> Finder(ASSET_PATH); \
		OUT_FIELD = Finder.Object;                                          \
	}                                                                       \
	while (false)
