rc ..\res\Stampante.rc
cl ..\src\main.cpp ..\lib\lodepng.cpp ..\lib\files.cpp ..\res\Stampante.res  -Zi gdi32.lib kernel32.lib user32.lib comdlg32.lib winmm.lib -FAcsu -o stampante.exe
