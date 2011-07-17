@echo off
cl /Zi /Ox /Oi /Ot /favor:INTEL64 /EHs-c- /GL /MT /GS- /GR- /W3 /nologo /D "X64" /D "NDEBUG" /TP ../main.cpp ../movegen.cpp ../position.cpp ../search.cpp ../psqTables.cpp ../windows.cpp ../evaluation.cpp ../tests.cpp ../utilities.cpp /link /OUT:"GarboChess3.exe" /INCREMENTAL:NO /DEBUG /FIXED:NO
copy /Y garbochess3.exe ..
REM pushd ..
REM garbochess3.exe
REM popd