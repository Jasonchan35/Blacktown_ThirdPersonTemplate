#pragma once

struct MyEnum
{
	MyEnum() = delete;
public:
	template<class T> using IntType = typename std::underlying_type<T>::type;

	template<class T> inline static
	typename IntType<T> ToInt(const T& V)
	{
		static_assert(std::is_enum_v<T>);
		return static_cast<typename IntType<T>::Type>(V);
	}

	template<class T> inline static FName	ToFName	(const T& V)
	{
		static_assert(std::is_enum_v<T>);
		return StaticEnum<T>()->GetNameByValue(ToInt(V));
	}
	
	template<class T> inline static FString	ToString(const T& V)
	{
		static_assert(std::is_enum_v<T>);
		auto* E = StaticEnum<T>();
		auto Name = E->GetNameByValue(static_cast<int64>(V));
		auto TypeNameLen = E->GetFName().GetStringLength();
		return Name.ToString().RightChop(TypeNameLen + 2); // remove 'EnumType::'
	}
};

