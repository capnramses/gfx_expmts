@echo off

REM this is derived from Kevin Moran's build.bat

REM set COMPILER_FLAGS=/MTd /nologo /Gm- /EHa- /GR- /fp:fast /Od /Oi /W4 /wd4201 /wd4100 /wd4189 /wd4514 /wd4820 /wd4505 /FC /Fm /Z7
REM set LINKER_FLAGS=/INCREMENTAL:NO /opt:ref
REM set SYSTEM_LIBS=user32.lib gdi32.lib winmm.lib
REM cl %COMPILER_FLAGS% ../main.c /link %LINKER_FLAGS% %SYSTEM_LIBS%

REM NOTE: Uncomment whichever one that you have installed!
REM call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x64
REM call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x64
REM call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
REM call "C:\Program Files (x86)\Microsoft Visual Studio 13.0\VC\vcvarsall.bat" x64
REM call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
REM call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

REM The /Z7 option produces object files that also contain full symbolic debugging information for use with the debugger
REM /Fc Causes the compiler to display the full path of source code files passed to the compiler in diagnostics.
REM /Fm outputs a map file
REM /Fd outputs a PDB file /MTd link with LIBCMTD.LIB debug lib /F<num> set stack size /W<n> set warning level (default n=1) /Wall enable all warnings /wd<n> disable warning n
set COMPILER_FLAGS=/MTd /nologo /Wall /FC /Fm /Z7 /wd4061 /wd4100 /wd4201 /wd4204 /wd4255 /wd4365 /wd4668 /wd4820 /wd5100
set LINKER_FLAGS=/INCREMENTAL:NO /opt:ref

set BUILD_DIR=".\build"
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
pushd %BUILD_DIR%

cl %COMPILER_FLAGS% ../main.c ../d3dutils.c /I ./ /link %LINKER_FLAGS%

popd
echo Done
