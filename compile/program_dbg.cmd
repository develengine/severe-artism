cl /std:c11 /W4 /wd5105 /wd4706 /w44062 /nologo /EHsc ^
    /Feprogram ^
    src/windows/bag_win32.c src/windows/audio_wasapi.c src/glad/gl.c ^
    src/main.c src/utils.c src/res.c src/animation.c src/core.c src/audio.c src/gui.c ^
    /Isrc /Isrc/glad/include ^
    /D_DEBUG /D_CRT_SECURE_NO_WARNINGS ^
    User32.lib Gdi32.lib Opengl32.lib Ole32.lib ksuser.lib

@echo off
