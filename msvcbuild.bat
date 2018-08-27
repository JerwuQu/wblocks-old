@rem Script to build wblocks with MSVC.
@rem Inspired by the LuaJIT build file with the same name.

@if not defined INCLUDE goto :VSERR

@rem Vars
@setlocal
@set WB_CL=cl /nologo /c /O2
@set WB_LINK=link /NOLOGO
@set WB_LJ=ext\LuaJIT\src
@set WB_TOML=ext\tomlc99\toml.c

@rem Output dirs
@if not exist bin mkdir bin
@if not exist obj\wblocks mkdir obj\wblocks
@if not exist obj\wblocks-dll mkdir obj\wblocks-dll

@rem Build wblocks
%WB_CL% /Foobj\wblocks\ %WB_TOML%
@if errorlevel 1 goto :BUILDERR
%WB_CL% /W3 /I %WB_LJ% /Foobj\wblocks\ wblocks\*.c
@if errorlevel 1 goto :BUILDERR
%WB_LINK% /OUT:bin\wblocks.exe /LIBPATH:%WB_LJ% user32.lib gdi32.lib lua51.lib luajit.lib obj\wblocks\*.obj
@if errorlevel 1 goto :BUILDERR

@rem Build wblocks-dll
%WB_CL% /W3 /Foobj\wblocks-dll\ wblocks-dll\*.c
@if errorlevel 1 goto :BUILDERR
%WB_LINK% /DLL /IMPLIB:%TEMP%\wblocks-dll.lib /OUT:bin\wblocks-dll.dll user32.lib obj\wblocks-dll\*.obj
@if errorlevel 1 goto :BUILDERR

@rem Copy files into bin
copy /Y %WB_LJ%\lua51.dll bin\
copy example\wblocks.toml bin\
copy example\example.lua bin\

@goto :END

@rem Error handling
:BUILDERR
@echo --------------------
@echo --------------------
@echo --- Build failed ---
@echo --------------------
@echo --------------------
@goto :END

:VSERR
@echo You must open a "Visual Studio .NET Command Prompt" to run this script

:END
