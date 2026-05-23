@echo off
cd /d "%~dp0"
gnuplot plot.gnuplot
if errorlevel 1 (
    echo.
    echo ===============================================
    echo Ошибка: gnuplot не найден или не установлен!
    echo ===============================================
    echo Установите gnuplot с http://www.gnuplot.info/
    echo или выполните: winget install gnuplot
    echo ===============================================
    pause
) else (
    echo.
    echo ===============================================
    echo График успешно построен: ga_plot.png
    echo ===============================================
)
