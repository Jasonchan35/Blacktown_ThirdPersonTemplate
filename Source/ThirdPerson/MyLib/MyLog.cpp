#include "MyLog.h"

DECLARE_LOG_CATEGORY_EXTERN(MyLog, Log, All);
DEFINE_LOG_CATEGORY(MyLog)

void MyLogger::Log(EMyLogLevel Lv, const FString& Msg)
{
	FColor Color = FColor::Red;
	auto logVerbosity = ELogVerbosity::Error;

	switch (Lv)
	{
		case EMyLogLevel::Info:
		{
			Color = FColor::White;
			UE_LOG(MyLog, Display, TEXT("%s"), *Msg);
		} break;

		case EMyLogLevel::Warning:
		{
			Color = FColor::Orange;
			UE_LOG(MyLog, Warning, TEXT("%s"), *Msg);
		} break;

		case EMyLogLevel::Error:
		{
			Color = FColor::Red;
			UE_LOG(MyLog, Error, TEXT("%s"), *Msg);
		} break;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3, Color, Msg);
	}
}

void MyLogger::LogOnScreen(const char* Filename, int Line, const FString& Msg)
{
	if (GEngine)
	{
		uint64 Hash = GetTypeHash(FName(Filename));
		uint64 Key = (Hash << 32) | Line;
		GEngine->AddOnScreenDebugMessage(Key, 3, FColor::Turquoise, Msg);
	}
}