#pragma once

struct MyType
{
	MyType() = delete;
private:
	template<class T> struct RemoveRef_;

public:
	template<class T> using RemoveRef = RemoveRef_<T>::Type;
};

template<class T> struct MyType::RemoveRef_<TObjectPtr<T>		> { using Type = T; };
template<class T> struct MyType::RemoveRef_<TSoftObjectPtr<T>	> { using Type = T; };
template<class T> struct MyType::RemoveRef_<TSubclassOf<T>		> { using Type = T; };
template<class T> struct MyType::RemoveRef_<T*					> { using Type = T; };


