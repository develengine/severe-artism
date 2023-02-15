
cl /O2 /std:c11 /W4 /wd5105 /wd4706 /w44062 /nologo /EHsc /Feprogram win32/bag_win32.c win32/audio_win32.c src/main.c src/utils.c src/res.c src/animation.c src/terrain.c src/core.c src/state.c src/levels.c src/audio.c src/gui.c src/splash.c src/settings.c glad/src/gl.c /Isrc /Iglad/include /D_DEBUG /D_CRT_SECURE_NO_WARNINGS User32.lib Gdi32.lib Opengl32.lib Ole32.lib ksuser.lib

@echo off
