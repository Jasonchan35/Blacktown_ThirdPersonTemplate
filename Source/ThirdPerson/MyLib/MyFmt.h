#pragma once

#define FMT_HEADER_ONLY 1
#define FMT_EXCEPTIONS 0
#include "fmt/core.h"

#include "MyEnum.h"

using MyFmt_Context = fmt::wformat_context; // fmt::basic_format_context<JxFormat_FStringBackInserter, TCHAR>;

class MyFmt_FStringBackInserter
{
	using ThisClass = MyFmt_FStringBackInserter;

public:
	MyFmt_FStringBackInserter() = default;
	explicit MyFmt_FStringBackInserter(FString& Output_) noexcept
		: Output(&Output_) {}

	ThisClass& operator=(const char& Value)
	{
		Output->AppendChar(Value);
		return *this;
	}
	ThisClass& operator=(const wchar_t& Value)
	{
		Output->AppendChar(Value);
		return *this;
	}

	ThisClass& operator*() noexcept { return *this; }
	ThisClass& operator++() noexcept { return *this; }
	ThisClass  operator++(int) noexcept { return *this; }

private:
	FString* Output = nullptr;
};

template <class... ARGS>
inline void MyFmtTo(FString& OutString, const TCHAR* Format, const ARGS&... Args)
{
	FMT_TRY
	{
		fmt::format_to(MyFmt_FStringBackInserter(OutString), Format, Args...);
	}
	FMT_CATCH(...)
	{
		UE_LOG(LogTemp, Error, TEXT("Exception in JxFormat %s"), Format);
	}
}

template <class... ARGS>
inline FString MyFmt(const TCHAR* Format, const ARGS&... Args)
{
	FString Tmp;
	MyFmtTo(Tmp, Format, Args...);
	return Tmp;
}

//------

template <>
struct fmt::formatter<FStringView, TCHAR>
{
	auto parse(fmt::wformat_parse_context& Parse) { return Parse.begin(); }

	auto format(const FStringView& v, MyFmt_Context& Context)
	{
		auto it = *Context.out();
		for (const auto& c : v)
		{
			it = c;
			it++;
		}
		return Context.out();
	}
};

using MyFmt_FormatterBase = fmt::formatter<FStringView, TCHAR>;

template <>
struct fmt::formatter<FString, TCHAR> : public MyFmt_FormatterBase
{
	auto format(const FString& v, MyFmt_Context& ctx)
	{
		return MyFmt_FormatterBase::format(v, ctx);
	}
};

template <>
struct fmt::formatter<FName, TCHAR> : public MyFmt_FormatterBase
{
	auto format(const FName& v, MyFmt_Context& ctx)
	{
		return MyFmt_FormatterBase::format(v.ToString(), ctx);
	}
};

// enum class
template <class T, class CH>
struct fmt::formatter<T, CH, std::enable_if_t<std::is_enum_v<T>>> : public MyFmt_FormatterBase
{
	auto format(const T& V, MyFmt_Context& Context)
	{
		return MyFmt_FormatterBase::format(MyEnum::ToString(V), Context);
	}
};

template <class E>
struct fmt::formatter<UE::Math::TVector2<E>, TCHAR> : public MyFmt_FormatterBase
{
	auto format(const UE::Math::TVector2<E>& v, MyFmt_Context& ctx)
	{
		return fmt::format_to(ctx.out(), TEXT("[X={}, Y={}]"), v.X, v.Y);
	}
};

template <class E>
struct fmt::formatter<UE::Math::TVector<E>, TCHAR> : public MyFmt_FormatterBase
{
	auto format(const UE::Math::TVector<E>& v, MyFmt_Context& ctx)
	{
		return fmt::format_to(ctx.out(), TEXT("[X={}, Y={}, Z={}]"), v.X, v.Y, v.Z);
	}
};

template <class E>
struct fmt::formatter<UE::Math::TVector4<E>, TCHAR> : public MyFmt_FormatterBase
{
	auto format(const UE::Math::TVector4<E>& v, MyFmt_Context& ctx)
	{
		return fmt::format_to(ctx.out(), TEXT("[X={}, Y={}, Z={}, W={}]"), v.X, v.Y, v.Z, v.W);
	}
};

template <>
struct fmt::formatter<FVector_NetQuantize, TCHAR> : public MyFmt_FormatterBase
{
	auto format(const FVector_NetQuantize& v, MyFmt_Context& ctx)
	{
		return fmt::format_to(ctx.out(), TEXT("[X={}, Y={}, Z={}]"), v.X, v.Y, v.Z);
	}
};

template <class E>
struct fmt::formatter<UE::Math::TRotator<E>, TCHAR> : public MyFmt_FormatterBase
{
	auto format(const UE::Math::TRotator<E>& v, MyFmt_Context& ctx)
	{
		return fmt::format_to(ctx.out(), TEXT("Rot[Pitch={}, Yaw={}, Roll={}]"), v.Pitch, v.Yaw, v.Roll);
	}
};
