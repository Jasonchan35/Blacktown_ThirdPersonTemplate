#pragma once

#define FMT_HEADER_ONLY 1
#define FMT_EXCEPTIONS 0
#include "fmt/core.h"

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
	using Super = fmt::formatter<FStringView, TCHAR>;
	auto format(const FString& v, MyFmt_Context& ctx)
	{
		return Super::format(v, ctx);
	}
};