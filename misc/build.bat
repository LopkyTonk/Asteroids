@echo off
IF NOT EXIST build mkdir build 
pushd build 

set common_compiler_flags= /MTd /Od /nologo /std:c++17 /fp:fast /Gm- /GR- /EHa- /WX /W4 /wd4312 /wd4003 /wd4100 /wd4189 /wd4201 /wd4505 /wd4324 /FC /Z7 
set common_linker_flags= /INCREMENTAL:no gdi32.lib user32.lib ws2_32.lib  /MACHINE:X64

cl %common_compiler_flags% ..\code\asteroids.cpp %common_linker_flags% d3d12.lib dxgi.lib ole32.lib winmm.lib /link

dxc rootsig_1_0 ..\code\standard.hlsl /EMainVS -Tvs_6_0 -Fo standard.vs
dxc rootsig_1_0 ..\code\standard.hlsl /EMainPS -Tps_6_0 -Fo standard.ps

popd
