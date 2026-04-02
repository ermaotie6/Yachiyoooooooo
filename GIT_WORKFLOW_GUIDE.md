# 📊 Yachiyo Git 工作流指南

**仓库位置**: `d:\Personal_Project\Yachiyo`  
**初始化日期**: 2026-04-03  
**主分支**: `main`  
**当前版本**: `v1.0.0`

---

## 📋 目录

- [分支策略](#分支策略)
- [提交规范](#提交规范)
- [工作流程](#工作流程)
- [标签管理](#标签管理)
- [常用命令](#常用命令)
- [团队协作](#团队协作)
- [故障排查](#故障排查)

---

## 🌳 分支策略

### 主要分支

#### `main` - 生产分支
```
用途: 生产环境代码
规则:
  ✅ 只接受来自 develop 或 hotfix 的 PR
  ✅ 每个提交都应该对应一个版本标签
  ✅ 严格受保护，禁止直接推送
  ✅ 必须通过 PR 审核

命名规范: v1.0.0 (主版本.次版本.修复版本)
```

#### `develop` - 开发分支
```
用途: 集成最新开发功能
规则:
  ✅ 从 feature/* 分支合并
  ✅ 作为测试和集成环境
  ✅ 相对较稳定，但可能包含未发布的功能
  ✅ 定期发布到 main

更新频率: 每周或每两周发布一次
```

### 功能分支

#### `feature/*` - 功能开发
```
命名: feature/<描述>
示例: 
  - feature/openclaw-integration
  - feature/gptsovits-integration
  - feature/live2d-animation
  - feature/user-authentication

流程:
  1. 从 develop 分支切出
  2. 开发功能
  3. 提交 PR 到 develop
  4. 审核通过后合并
  5. 删除功能分支

代码审核:
  - 最少 2 个批准
  - CI/CD 必须通过
  - 覆盖率 > 80%
```

#### `bugfix/*` - 错误修复
```
命名: bugfix/<描述>
示例:
  - bugfix/openclaw-timeout-issue
  - bugfix/gptsovits-audio-quality
  - bugfix/live2d-animation-sync

流程:
  1. 从 develop 分支切出
  2. 修复错误
  3. 提交 PR 到 develop
  4. 通过审核后合并

紧急修复:
  如果是生产环境的严重错误:
  1. 从 main 切出 hotfix/* 分支
  2. 修复问题
  3. 合并到 main 和 develop
```

#### `hotfix/*` - 热修复
```
命名: hotfix/<版本号>-<描述>
示例:
  - hotfix/v1.0.1-openclaw-crash
  - hotfix/v1.0.2-authentication-bypass

用途:
  - 生产环境的严重问题
  - 需要立即发布
  - 不能等待定期发布

流程:
  1. 从 main 分支切出
  2. 修复问题
  3. 更新版本号
  4. 合并到 main，标记新版本
  5. 合并到 develop 同步
  6. 删除分支
```

### 分支结构图

```
main (生产)
  ↑
  ├─ PR from develop (定期发布)
  │
  └─ hotfix/* (紧急修复)
       ↓
      [修复] → PR → main (标记版本)
                     ↓
                   develop (合并回来)

develop (集成/测试)
  ↑
  ├─ PR from feature/* (功能完成)
  ├─ PR from bugfix/* (错误修复)
  └─ PR from hotfix (热修复)
       ↓
      [合并] → 自动部署到测试环境

feature/* (功能开发)
  →─ [开发] →─ PR →─ develop
             [审核]

bugfix/* (错误修复)
  →─ [修复] →─ PR →─ develop
             [审核]
```

---

## ✍️ 提交规范

### 提交信息格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### 类型 (type)

| 类型 | 用途 | 示例 |
|------|------|------|
| `feat` | 新功能 | feat(openclaw): add parameter generation |
| `fix` | 错误修复 | fix(gptsovits): resolve timeout issue |
| `docs` | 文档更新 | docs: update README.md |
| `style` | 代码格式 | style: format code with clang-format |
| `refactor` | 代码重构 | refactor(auth): simplify token validation |
| `perf` | 性能优化 | perf(cache): implement LRU caching |
| `test` | 测试相关 | test(api): add integration tests |
| `chore` | 工具、依赖 | chore(deps): update cmake to 3.25 |
| `ci` | CI/CD 配置 | ci: add github actions workflow |

### 范围 (scope)

```
backend      - 后端代码
frontend     - 前端代码
docs         - 文档
infrastructure - 基础设施
openclaw     - OpenClaw 集成
gptsovits    - GPT-SoVITS 集成
live2d       - Live2D 动画
api          - API 接口
auth         - 认证系统
cache        - 缓存系统
database     - 数据库
```

### 主题 (subject)

```
✅ 规则:
  - 以小写开头
  - 不超过 50 个字符
  - 使用现在时态 ("add" 而不是 "added")
  - 不以句号结尾

❌ 错误示例:
  - "Fixed the bug"          (大写开头)
  - "add new openclaw parameter system" (太长)
  - "updates docs."          (以句号结尾)

✅ 正确示例:
  - "add openclaw parameter generation"
  - "fix timeout in gptsovits api call"
  - "update documentation for deployment"
```

### 内容 (body)

```
解释做了什么以及为什么这样做。

示例:
  Implement parameter generation in OpenClaw integration.
  
  The backend now receives emotion analysis and generates
  voice parameters (speed, pitch, energy) and animation
  parameters (expression, gesture) for downstream systems.
  
  This enables synchronized audio and animation playback.
```

### 页脚 (footer)

```
用于关联问题或 breaking changes。

示例:
  Fixes #123
  Closes #456
  Breaking-Change: OpenClaw API now requires language parameter
```

### 完整提交示例

```
feat(openclaw): add emotion-aware parameter generation

Implement emotion analysis in OpenClaw integration that generates
voice and animation parameters based on user input and conversation
context.

Features:
- Emotion classification (happy, sad, neutral, etc.)
- Parameter generation for GPT-SoVITS (speed, pitch, energy)
- Animation parameters for Live2D (expression, gesture, timing)
- Context-aware generation using conversation history

Fixes #78
```

---

## 🔄 工作流程

### 开发新功能的完整流程

#### 1. 创建功能分支

```bash
# 切到 develop 分支
git checkout develop

# 获取最新代码
git pull origin develop

# 创建功能分支
git checkout -b feature/openclaw-parameter-generation
```

#### 2. 开发和提交

```bash
# 进行开发...
# 修改文件

# 查看更改
git status

# 暂存更改
git add <file>
# 或全部添加
git add .

# 提交更改
git commit -m "feat(openclaw): implement parameter generation logic"

# 继续开发、提交...
git commit -m "test(openclaw): add unit tests for parameter generation"
git commit -m "docs(openclaw): update parameter documentation"
```

#### 3. 推送到远程

```bash
# 首次推送
git push -u origin feature/openclaw-parameter-generation

# 后续推送
git push origin feature/openclaw-parameter-generation
```

#### 4. 创建 Pull Request

```
在 GitHub/GitLab 上:
1. 点击 "New Pull Request"
2. 选择:
   - Base: develop
   - Compare: feature/openclaw-parameter-generation
3. 填写 PR 描述:
   标题: feat(openclaw): add parameter generation
   描述: 
     ## 做了什么
     - 实现了参数生成逻辑
     - 支持多种情感
     - 集成了缓存机制
     
     ## 测试
     - ✅ 单元测试通过
     - ✅ 集成测试通过
     - ✅ 代码覆盖率 >85%
     
     ## 相关问题
     Fixes #78
4. 提交 PR
```

#### 5. 代码审核

```
审核人员检查:
- ✅ 代码质量
- ✅ 测试覆盖率
- ✅ 文档更新
- ✅ 提交信息规范

如果需要修改:
  git commit -m "fix: address code review comments"
  git push origin feature/openclaw-parameter-generation
```

#### 6. 合并

```bash
# 审核通过后，在 GitHub/GitLab 点击 "Merge"
# 或本地合并:

git checkout develop
git pull origin develop
git merge --no-ff feature/openclaw-parameter-generation
git push origin develop

# 删除功能分支
git branch -d feature/openclaw-parameter-generation
git push origin --delete feature/openclaw-parameter-generation
```

### 发布新版本的流程

```bash
# 1. 从 develop 创建发布版本
git checkout -b release/v1.1.0 develop

# 2. 更新版本号
# 编辑 CMakeLists.txt, package.json 等
git commit -m "chore: bump version to 1.1.0"

# 3. 合并到 main
git checkout main
git pull origin main
git merge --no-ff release/v1.1.0
git tag v1.1.0 -m "Release version 1.1.0"

# 4. 合并回 develop
git checkout develop
git merge --no-ff release/v1.1.0

# 5. 推送
git push origin main develop --tags

# 6. 删除发布分支
git branch -d release/v1.1.0
git push origin --delete release/v1.1.0
```

### 修复生产问题的流程

```bash
# 1. 创建热修复分支
git checkout -b hotfix/v1.0.1-openclaw-crash main

# 2. 修复问题
# 编辑相关文件
git commit -m "fix(openclaw): fix null pointer crash in parameter generation"

# 3. 更新版本
git commit -m "chore: bump version to 1.0.1"

# 4. 合并到 main
git checkout main
git merge --no-ff hotfix/v1.0.1-openclaw-crash
git tag v1.0.1 -m "Hotfix: crash in parameter generation"

# 5. 合并到 develop
git checkout develop
git merge --no-ff hotfix/v1.0.1-openclaw-crash

# 6. 推送
git push origin main develop --tags

# 7. 删除分支
git branch -d hotfix/v1.0.1-openclaw-crash
git push origin --delete hotfix/v1.0.1-openclaw-crash
```

---

## 🏷️ 标签管理

### 版本号规范 (Semantic Versioning)

```
v<MAJOR>.<MINOR>.<PATCH>

示例: v1.2.3

MAJOR: 不兼容的 API 更改
  v0.0.0 → v1.0.0  新的主要功能或架构
  
MINOR: 向后兼容的新功能
  v1.0.0 → v1.1.0  新功能、性能改进
  
PATCH: 错误修复和补丁
  v1.1.0 → v1.1.1  错误修复、小改进
```

### 创建标签

```bash
# 创建轻量标签
git tag v1.0.0

# 创建注解标签 (推荐)
git tag v1.0.0 -m "Release version 1.0.0"

# 为过去的提交创建标签
git tag v1.0.0 <commit-hash>

# 推送标签
git push origin v1.0.0      # 推送单个标签
git push origin --tags      # 推送所有标签
```

### 查看标签

```bash
# 列出所有标签
git tag -l

# 查看特定标签信息
git show v1.0.0

# 按日期排序显示标签
git tag -l --sort=-version:refname
```

### 发布版本清单

```
标记版本前检查:
  ☑️ 所有功能已完成
  ☑️ 所有测试通过
  ☑️ 文档已更新
  ☑️ 性能基准已验证
  ☑️ 代码审核已通过
  ☑️ 版本号已更新
  ☑️ CHANGELOG 已更新
  ☑️ 所有关键问题已解决
```

---

## 🔧 常用命令

### 查看状态和历史

```bash
# 查看当前分支和未暂存的更改
git status

# 查看提交历史
git log --oneline

# 查看详细提交历史
git log --graph --oneline --all --decorate

# 查看特定文件的历史
git log --oneline -- <file>

# 查看某个提交的更改
git show <commit-hash>

# 查看两个分支之间的差异
git diff main develop

# 查看当前分支的分支图
git log --graph --oneline --all --decorate
```

### 分支操作

```bash
# 列出所有分支
git branch -a

# 创建新分支
git branch <branch-name>

# 切换分支
git checkout <branch-name>
# 或 (Git 2.23+)
git switch <branch-name>

# 创建并切换到新分支
git checkout -b <branch-name>

# 删除本地分支
git branch -d <branch-name>

# 强制删除本地分支
git branch -D <branch-name>

# 删除远程分支
git push origin --delete <branch-name>

# 重命名分支
git branch -m <old-name> <new-name>
```

### 提交相关

```bash
# 查看未暂存的更改
git diff

# 查看暂存的更改
git diff --staged

# 暂存文件
git add <file>

# 暂存所有更改
git add .

# 取消暂存文件
git reset <file>

# 提交更改
git commit -m "<message>"

# 修改最后的提交
git commit --amend

# 查看提交历史
git log --oneline

# 撤销最后一次提交 (保留更改)
git reset --soft HEAD~1

# 撤销最后一次提交 (丢弃更改)
git reset --hard HEAD~1

# 恢复已删除的文件
git restore <file>
```

### 远程仓库

```bash
# 添加远程仓库
git remote add origin <url>

# 查看远程仓库
git remote -v

# 拉取最新代码
git pull origin <branch>

# 推送到远程
git push origin <branch>

# 拉取所有分支
git fetch --all

# 更新本地分支列表
git remote prune origin
```

### 合并和变基

```bash
# 合并分支
git merge <branch>

# 变基分支 (整理历史)
git rebase <branch>

# 中止合并/变基
git merge --abort
git rebase --abort

# 交互式变基
git rebase -i HEAD~<n>
```

### 紧急操作

```bash
# 暂存当前更改 (保存工作现场)
git stash

# 列出所有暂存
git stash list

# 应用最近的暂存
git stash pop

# 清除所有暂存
git stash clear

# 查看暂存内容
git stash show -p stash@{0}
```

---

## 👥 团队协作

### 代码审核规范

```
每个 PR 必须满足:

1. 提交信息规范
   ✅ 格式正确
   ✅ 描述清晰
   ✅ 类型正确

2. 代码质量
   ✅ 代码风格一致
   ✅ 无重复代码
   ✅ 函数/类有文档
   ✅ 错误处理完整

3. 测试覆盖
   ✅ 新功能有单元测试
   ✅ 测试覆盖率 > 80%
   ✅ 所有测试通过
   ✅ 集成测试通过

4. 文档更新
   ✅ README 已更新
   ✅ API 文档已更新
   ✅ 架构文档已更新

5. 性能影响
   ✅ 性能下降 < 5%
   ✅ 内存占用 < 10%
   ✅ 响应时间可接受

6. 安全性
   ✅ 无安全漏洞
   ✅ 敏感信息已加密
   ✅ 输入已验证
```

### 冲突解决

```bash
# 发生冲突时的步骤:

1. 识别冲突文件
   git status

2. 编辑冲突文件
   <<<<<<< HEAD
   现有代码
   =======
   新代码
   >>>>>>> branch-name
   
   修改为正确的代码

3. 标记为已解决
   git add <file>

4. 完成合并
   git commit -m "Resolve merge conflict in <file>"

5. 推送
   git push origin <branch>
```

### 团队最佳实践

```
✅ DO:
  - 定期 pull 最新代码
  - 提交前运行所有测试
  - 在 PR 中详细描述更改
  - 尽快响应代码审核评论
  - 保持分支同步
  - 频繁的小提交
  - 有意义的提交信息

❌ DON'T:
  - 不要直接推送到 main
  - 不要提交大型二进制文件
  - 不要提交敏感信息 (密钥、密码)
  - 不要强制推送到共享分支
  - 不要过时的分支不删除
  - 不要提交调试代码
  - 不要没有测试的功能
```

---

## 🔍 故障排查

### 常见问题

#### 提交到错误的分支

```bash
# 问题: 提交到了 main 而不是 feature
# 解决:

# 1. 创建正确的分支
git branch feature/correct-branch

# 2. 重置 main 到上一个提交
git reset --hard HEAD~1

# 3. 切换到新分支
git checkout feature/correct-branch

# 4. 推送
git push origin feature/correct-branch
```

#### 需要撤销提交

```bash
# 问题: 需要撤销最后一次提交

# 选项 1: 保留更改 (创建新提交)
git revert HEAD

# 选项 2: 撤销提交和更改
git reset --hard HEAD~1

# 选项 3: 修改提交
git commit --amend
```

#### 恢复已删除的分支

```bash
# 查看最近的操作
git reflog

# 恢复分支
git checkout -b <branch-name> <commit-hash>
```

#### 修复错误的提交信息

```bash
# 修改最后一次提交的信息
git commit --amend --no-edit

# 修改提交信息并重新编辑
git commit --amend -m "新的提交信息"

# 修改历史提交 (危险操作)
git rebase -i HEAD~<n>
# 然后将 "pick" 改为 "reword"
```

#### 误删除文件恢复

```bash
# 查看删除记录
git log -- <file>

# 恢复文件
git restore <file>
# 或从特定提交恢复
git restore --source=<commit> <file>
```

### 调试命令

```bash
# 查看谁修改了这一行
git blame <file>

# 查看哪些提交改变了某个功能
git log -S "<function-name>" -- <file>

# 查看分支的创建时间
git log --oneline --graph --all

# 检查某个文件在哪个提交被修改
git log --oneline -- <file>

# 找出引入 bug 的提交 (二分查找)
git bisect start
git bisect bad HEAD
git bisect good <known-good-commit>
```

---

## 📊 仓库统计

### 当前状态

```
仓库路径: d:\Personal_Project\Yachiyo
初始化时间: 2026-04-03
当前分支: main

分支列表:
  - main (生产)
  - develop (开发)
  - feature/openclaw-integration
  - feature/gptsovits-integration

版本标签:
  - v1.0.0 (当前)

提交统计:
  - 总提交数: 1
  - 主要贡献者: Yachiyo Developer
```

### 仓库结构

```
Yachiyo/
├─ .git/                    # Git 元数据
├─ .gitignore               # 忽略文件
├─ backend/                 # 后端代码
├─ frontend/                # 前端代码
├─ docs/                    # 文档
├─ resources/               # 资源文件
└─ README.md                # 项目说明
```

---

## 📞 获取帮助

```bash
# 查看 git 帮助
git help <command>

# 示例
git help commit
git help branch
git help merge
```

---

**Git 仓库设置完成！开始使用 Git 跟踪代码更改吧！** 🚀
