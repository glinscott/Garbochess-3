cl /Ox /Oi /Ot /favor:INTEL64 /EHs-c- /GL /MT /GS- /GR- /W3 /nologo /D "X64" /D "NDEBUG" /TP ../main.cpp ../movegen.cpp ../position.cpp ../windows.cpp /link /OUT:"GarboChess3.exe" /INCREMENTAL:NO
copy /Y garbochess3.exe ..
pushd ..
garbochess3.exe
popd