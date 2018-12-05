# wblocks
A taskbar thingy

Make sure to take the 64-bit version if you're running 64-bit Windows. Don't just take the 32-bit one.

## Building Steps
0. Clone the git repo and include external dependencies.
1. Start a console with Visual C/C++ environment variables (which can be done by running `C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64\vcvars64.bat` in a cmd instance).
2. CD to `ext/LuaJIT/src` and run `msvcbuild.bat` to build LuaJIT.
3. CD back to wblocks root and run `msvcbuild.bat` to build wblocks.
4. Run `wblocks.exe` in `bin`.
5. Write some cool blocks in Lua.
6. ???
7. Profit

## License
GNU GPLv3
