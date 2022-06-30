	@echo off
    setlocal
    FOR /F "usebackq" %%A IN ('out\mainloader.bin') DO set size3=%%~zA
    FOR /F "usebackq" %%A IN ('out\mainloader.lz') DO set size2=%%~zA
    FOR /F "usebackq" %%A IN ('out\loader.bin') DO set size=%%~zA
    SET  /A preSize = size - size2
    echo.___Final blob sizes____________
    echo.Mainloader: %size2% bytes (uncompressed %size3% bytes)
    echo.Preloader : %preSize% bytes
    echo.Total     : %size% bytes
    echo.
	@echo on
