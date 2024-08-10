#include "MyConsole.h"
#include "MyLog.h"

void MyConsole::SetVar(const TCHAR* Name, int Value)
{
	auto* V = IConsoleManager::Get().FindConsoleVariable(Name);
	if (!V)
	{
		MY_LOG_ERROR(L"SetConsoleVar() Cannot find console var '{}'", Name);
		return;
	}
	V->Set(Value);
}

bool MyConsole::GetVar(const TCHAR* Name, int& OutValue)
{
	auto* V = IConsoleManager::Get().FindConsoleVariable(Name);
	if (!V)
	{
		MY_LOG_ERROR(L"Cannot find console var '{}'", Name);
		return false;
	}
	OutValue = V->GetInt();
	return true;
}

#if WITH_EDITOR
void MyConsole::EditorCmd(const TCHAR* Name)
{
	if (!GEditor)
		return;
	if (auto* World = GEditor->GetEditorWorldContext().World())
	{
		GEditor->Exec(World, Name);
	}
}
#endif
