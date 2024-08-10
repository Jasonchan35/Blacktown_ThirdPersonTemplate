#pragma once

struct MyConsole
{
	MyConsole() = delete;

	static void SetVar(const TCHAR* Name, int Value);
	static bool GetVar(const TCHAR* Name, int& OutValue);

#if WITH_EDITOR
	static void EditorCmd(const TCHAR* Name);
#endif
};
