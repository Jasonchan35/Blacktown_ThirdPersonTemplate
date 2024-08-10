@REM #change current directory to this file
%~d0
cd %~dp0

set uproject=%~dp0\ThirdPerson.uproject

"C:\Program Files\Epic Games\UE_5.4\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="%uproject%" -game -rocket -progress

@pause