call :CLEANUP
goto :EOF

:CLEANUP
del /F /Q *.ncb *.plog *.pdb *.lib *.ilk *.exp *.*.user *.obj *.sdf *.aps
del /F /Q /A:H  *.suo *.opensdf
rmdir /S /Q ipch
rmdir /S /Q Debug
rmdir /S /Q Release
