@echo off
setlocal

set WAR3HD_GL_DLL="opengl32.dll"
set WAR3HD_GL_DLL_PDB="opengl32.pdb"

set WAR3_GAME_EXE_PATH="C:\Users\Lampert\Documents\Warcraft-III-RoC"
set WAR3HD_GL_DLL_PATH="C:\Users\Lampert\Documents\GitHub\Repos\War3HD\Build\Debug"
set WAR3HD_SHADERS_PATH="C:\Users\Lampert\Documents\GitHub\Repos\War3HD\Source\Shaders"

del %WAR3_GAME_EXE_PATH%\%WAR3HD_GL_DLL%
del %WAR3_GAME_EXE_PATH%\%WAR3HD_GL_DLL_PDB%

mklink /H %WAR3_GAME_EXE_PATH%\%WAR3HD_GL_DLL% %WAR3HD_GL_DLL_PATH%\%WAR3HD_GL_DLL%
mklink /H %WAR3_GAME_EXE_PATH%\%WAR3HD_GL_DLL_PDB% %WAR3HD_GL_DLL_PATH%\%WAR3HD_GL_DLL_PDB%

mklink /J %WAR3_GAME_EXE_PATH%\NewShaders %WAR3HD_SHADERS_PATH%

rem Always exit with zero even if one of the commands above has failed, otherwise the VS post-build step will be marked as a failure.
rem mklink /J may fail if the directory junction already exists, which is benign.
exit /B 0
