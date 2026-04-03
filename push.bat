@echo off
REM Yachiyo GitHub Push Automation Script
REM 自动化 GitHub 推送流程

setlocal enabledelayedexpansion
cd /d "d:\Personal_Project\Yachiyo"

echo.
echo ========================================
echo Yachiyo GitHub Push Automation
echo ========================================
echo.

REM Step 1: Check status
echo [1/5] Checking Git Status...
git status --short > temp_status.txt
for /f %%A in ('find /v /c "" ^< temp_status.txt') do set count=%%A
echo Found %count% modified/new files

REM Step 2: Configure user
echo [2/5] Configuring Git User...
git config user.name "Yachiyo CI"
git config user.email "yachiyo@example.com"
git config --list --local | find "user.name"

REM Step 3: Stage all changes
echo [3/5] Staging All Changes...
git add -A
git diff --cached --stat | find ".py" > nul
if %ERRORLEVEL% EQU 0 echo ✓ Files staged successfully

REM Step 4: Create commit
echo [4/5] Creating Commit...
git commit -m "feat: Phase 2 implementation - WebSocket, Live2D, Audio, Database"
if %ERRORLEVEL% EQU 0 (
    echo ✓ Commit created successfully
    git log --oneline -1
) else (
    echo ⚠ Commit creation failed or no changes to commit
)

REM Step 5: Push to GitHub
echo [5/5] Pushing to GitHub...
git push -u origin main

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo ✓ Push Completed Successfully!
    echo ========================================
    echo.
    git log --oneline -1
) else (
    echo.
    echo ========================================
    echo ✗ Push Failed - Please check connection
    echo ========================================
    echo.
)

REM Cleanup
del temp_status.txt 2>nul

pause
