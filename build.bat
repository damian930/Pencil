:: =====================================================================================================
:: NOTES ABOUT THE CMD SYNTAX (just in case)
:: * To calculate the number of lines in the codebase:
::       cloc src --by-file 
::       OR 
::       cloc src --exclude-dir=__third_party,__retired_code --by-file
::        (doesnt include the third party code)
:: * In bat files, when creating variables using the 'set' command, the variables are just text, kind of.
::       Then when comparing var to a value later we use '==' operator. 
::       It compares 2 strings, unless the values inside are numerical. 
::       This means that we have to get the number of '"' around the values right, to not have this: ""value""=="value"
::       That is why when we set values using 'set' command, we set it without '"'.
::       But then when comparing via '==' we aply '"'. Like this: if "%var_name%"=="value".
::       This way we have the value from the var_name and the value be inside a single pair of '"'.
::       This sort of standart removes a lot of bugs when it coms to setting up a build file.
::       Sure, it would be better if Microsoft had made a better script lang, but this technique
::       definately makes the script file more coherent for both reading and writing. 
:: * "^" works like "/" in C macros. 
::       It does not work properly if there is anything after it, even white spaces.
:: =====================================================================================================

:: todo:
:: [ ] - have vcvars all not have to be called every time

@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"
cls

call vcvars64.bat > nul 

echo =====================================================================================================
echo =====================================================================================================

:: Getting command line arguments
for %%a in (%*) do set "%%a=1"

:: Type of build
if not "%release%"=="1" set debug=1
if "%release%"=="1"     set release=1
if "%debug%"=="1"       set debug=1
if "%debug%"=="1"       set release=

if "%debug%"=="1"       if "%release%"=="1" echo Build file is invalid, Debug and Release modes are both specified. && exit /b 1
if "%release%"=="1"     echo [release build]
if "%debug%"=="1"       echo [debug build]

:: Error strictness
set errors_to_alway_ignore=/wd4201 /wd4065
if "%release%"=="1"     set strict=1
if "%strict%"=="1"      (echo [strict]) else (echo [non strict])
if "%strict%"==""       set errors_to_ignore=/wd4189 /wd4100 /wd4505 %errors_to_alway_ignore%
if "%strict%"=="1"      set errors_to_ignore=%errors_to_alway_ignore%

:: Pre processor defines
if "%debug%"=="1"                           set pre_processor_defines=/D"DEBUG_MODE"
if "%release%"=="1"                         set pre_processor_defines=/D"RELEASE_MODE"
if "%dont_assert_handle_later_macros%"=="1" set pre_processor_defines=%pre_processor_defines% /D"DONT_ASSERT_HANDLE_LATER_MACROS" && echo [UNRESOLVED_HANDLE_LATERs]

:: Common compiler flags
set common_compiler_flags=/nologo %errors_to_ignore% %pre_processor_defines% /INCREMENTAL:NO /I"../src" /W4 /MDd /FC /std:c++20 /permissive- 

:: Common linker flags
set common_linker_flags=/LIBPATH:"../src" /INCREMENTAL:NO

:: Compiler command <compiler> __path_to_file_to_compile__ __optional_extra_flags__ 
set debug_compile=call cl %common_compiler_flags% /Zi  
set release_compile=call cl %common_compiler_flags% /O2 

:: Linker command <linker> __stuff_to_link__ __optional_extra_flags__
set debug_link=/link %common_linker_flags% 
set release_link=/link %common_linker_flags% /opt:ref   

:: Final building commands
if "%debug%"=="1"   set compile=%debug_compile%   & set linker=%debug_link%
if "%release%"=="1" set compile=%release_compile% & set linker=%release_link%

if "%clean%"=="1" if exist build rmdir build /q /s >nul 2>&1 && echo [clean]
if not exist build mkdir build

:: Possible compilations targets
pushd build

if "%pencil%"=="1"         set build_succ=1 && %compile% ../src/main.cpp %linker% /OUT:"pencil.exe"
if "%glfw_learning%"=="1"  set build_succ=1 && %compile% ../src/__samples/glfw_learning.cpp %linker% /OUT:"glfw_learning.exe"

popd

if exist .vscode rmdir /s /q .vscode

if "%build_succ%"=="" echo Failed to build. No build target was specified



