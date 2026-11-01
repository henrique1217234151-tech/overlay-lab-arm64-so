@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\compilar-windows.ps1"
if errorlevel 1 (
  echo ERRO: veja a mensagem acima.
  pause
  exit /b 1
)
echo.
echo SAIDA: saida\liboverlay_plugin.so e saida\liboverlay_host.so
pause
