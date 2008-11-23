cl /Ox /Oi /Ot /favor:INTEL64 /EHs-c- /GL /MT /GS- /GR- /W3 /nologo /TP ../main.cpp ../movegen.cpp ../position.cpp ../windows.cpp /link /OUT:"GarboChess3.exe" /INCREMENTAL:NO
copy /Y garbochess3.exe ..
