@echo off

REM The /Z7 option produces object files that also contain full symbolic debugging information for use with the debugger
REM /Fc Causes the compiler to display the full path of source code files passed to the compiler in diagnostics.
REM /Fm outputs a map file
REM /Fd outputs a PDB file /MTd link with LIBCMTD.LIB debug lib /F<num> set stack size /W<n> set warning level (default n=1) /Wall enable all warnings /wd<n> disable warning n
set COMPILER_FLAGS=/MTd /nologo /Wall /FC /Fm /Z7 /wd4061 /wd4100 /wd4201 /wd4204 /wd4255 /wd4365 /wd4668 /wd4820 /wd5100
set LINKER_FLAGS=/INCREMENTAL:NO /opt:ref

set BUILD_DIR=".\build"
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
pushd %BUILD_DIR%

cl %COMPILER_FLAGS% ../main.cpp ../d3d11_utils.cpp /I ./ /link %LINKER_FLAGS%

popd
echo Done
