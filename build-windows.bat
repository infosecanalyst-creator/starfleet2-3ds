@echo off
rem SPDX-License-Identifier: GPL-2.0-or-later
rem Copyright (C) 2026 the starfleet2-3ds authors
rem Build the Star Fleet II 3DS app with devkitPro (C:\devkitpro). Log: build.log
cd /d "%~dp0"
echo build started > "%~dp0build.log"
del /q "%~dp0build.status" 2>nul
set "MSYSTEM=MSYS"
set "CHERE_INVOKING=1"
"C:\devkitpro\msys2\usr\bin\bash.exe" -l "%~dp0build-3ds.sh" >> "%~dp0build.log" 2>&1
if errorlevel 1 (echo FAILED> "%~dp0build.status") else (echo OK> "%~dp0build.status")
