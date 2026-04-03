@echo off
cd /d "d:\Personal_Project\Yachiyo"
echo === GIT ADD === >> "d:\Personal_Project\Yachiyo\__git_log.txt" 2>&1
git add -A >> "d:\Personal_Project\Yachiyo\__git_log.txt" 2>&1
echo === GIT STATUS === >> "d:\Personal_Project\Yachiyo\__git_log.txt" 2>&1
git status --short >> "d:\Personal_Project\Yachiyo\__git_log.txt" 2>&1
echo === GIT COMMIT === >> "d:\Personal_Project\Yachiyo\__git_log.txt" 2>&1
git commit -m "feat: Phase 3 - Baidu翻译/DeepSeek审核集成 + 部署文档 + 前端修复" >> "d:\Personal_Project\Yachiyo\__git_log.txt" 2>&1
echo === GIT PUSH === >> "d:\Personal_Project\Yachiyo\__git_log.txt" 2>&1
git push origin main >> "d:\Personal_Project\Yachiyo\__git_log.txt" 2>&1
echo === DONE === >> "d:\Personal_Project\Yachiyo\__git_log.txt" 2>&1
