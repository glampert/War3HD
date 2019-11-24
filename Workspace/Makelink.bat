
@echo off

set "WAR3HD_GL_DLL=opengl32.dll"
set "WAR3HD_GL_DLL_PDB=opengl32.pdb"

set "WAR3_GAME_EXE_PATH=C:\Users\Lampert\Documents\Warcraft-III-RoC\"
set "WAR3HD_GL_DLL_PATH=C:\Users\Lampert\Documents\Repos\War3HD\Build\Debug\"

del %WAR3_GAME_EXE_PATH%%WAR3HD_GL_DLL%
mklink /H %WAR3_GAME_EXE_PATH%%WAR3HD_GL_DLL% %WAR3HD_GL_DLL_PATH%%WAR3HD_GL_DLL%

copy /y %WAR3HD_GL_DLL_PATH%%WAR3HD_GL_DLL_PDB% %WAR3_GAME_EXE_PATH%%WAR3HD_GL_DLL_PDB%
