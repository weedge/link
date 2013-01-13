@echo off
@ ECHO.
pause
echo 正在运行，请稍等......
.\ParseToFile.exe parse_query51-100_param topics.51-100
echo. & pause
.\ParseToFile.exe parse_query101-150_param topics.101-150
echo. & pause 
.\ParseToFile.exe parse_query151-200_param topics.151-200
echo. & pause
