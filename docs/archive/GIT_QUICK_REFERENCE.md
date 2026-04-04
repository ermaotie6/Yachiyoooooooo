# 🚀 Git 快速参考

**针对 Yachiyo 项目的 Git 命令速查表**

---

## 日常命令

### 开始工作

```bash
# 获取最新代码
git pull origin develop

# 创建新功能分支
git checkout -b feature/your-feature-name
```

### 开发和提交

```bash
# 查看更改
git status

# 添加文件到暂存区
git add .

# 提交更改
git commit -m "feat(scope): your commit message"

# 推送到远程
git push origin feature/your-feature-name
```

### 完成功能

```bash
# 创建 Pull Request
# 在 GitHub/GitLab 上完成代码审核
# 合并到 develop 分支

# 删除本地分支
git branch -d feature/your-feature-name

# 删除远程分支
git push origin --delete feature/your-feature-name

# 更新本地 develop
git checkout develop
git pull origin develop
```

---

## 分支命令

| 操作 | 命令 |
|------|------|
| 列出所有分支 | `git branch -a` |
| 创建分支 | `git branch <name>` |
| 切换分支 | `git checkout <name>` |
| 创建并切换 | `git checkout -b <name>` |
| 删除分支 | `git branch -d <name>` |
| 强制删除 | `git branch -D <name>` |
| 重命名分支 | `git branch -m <old> <new>` |

---

## 提交命令

| 操作 | 命令 |
|------|------|
| 查看状态 | `git status` |
| 查看差异 | `git diff` |
| 添加文件 | `git add <file>` |
| 添加所有 | `git add .` |
| 提交 | `git commit -m "message"` |
| 修改提交 | `git commit --amend` |
| 查看日志 | `git log --oneline` |
| 查看详细图形 | `git log --graph --oneline --all` |

---

## 推送和拉取

| 操作 | 命令 |
|------|------|
| 拉取代码 | `git pull origin <branch>` |
| 推送代码 | `git push origin <branch>` |
| 首次推送 | `git push -u origin <branch>` |
| 拉取所有 | `git fetch --all` |
| 推送标签 | `git push origin --tags` |

---

## 提交信息模板

### 标准格式

```
feat(scope): short description

- Detailed bullet point 1
- Detailed bullet point 2

Fixes #123
```

### 常用类型

- `feat` - 新功能
- `fix` - 错误修复
- `docs` - 文档
- `style` - 代码格式
- `refactor` - 重构
- `perf` - 性能
- `test` - 测试
- `chore` - 工具/依赖
- `ci` - CI/CD

### 常用范围

- `openclaw` - OpenClaw 集成
- `gptsovits` - GPT-SoVITS 集成
- `live2d` - Live2D 动画
- `api` - API 接口
- `auth` - 认证
- `cache` - 缓存
- `frontend` - 前端
- `backend` - 后端

---

## 撤销和恢复

| 操作 | 命令 |
|------|------|
| 撤销提交 (保留更改) | `git reset --soft HEAD~1` |
| 撤销提交 (丢弃更改) | `git reset --hard HEAD~1` |
| 撤销更改到最后提交 | `git restore <file>` |
| 暂存当前工作 | `git stash` |
| 应用暂存 | `git stash pop` |
| 查看暂存列表 | `git stash list` |

---

## 合并和冲突

```bash
# 合并分支
git merge <branch>

# 中止合并
git merge --abort

# 解决冲突后继续
git add .
git commit -m "Resolve merge conflict"
```

---

## 标签管理

```bash
# 创建标签
git tag v1.0.0 -m "Release version 1.0.0"

# 列出标签
git tag -l

# 推送标签
git push origin v1.0.0

# 推送所有标签
git push origin --tags

# 查看标签详情
git show v1.0.0
```

---

## 工作流快速指南

### 开发新功能

```bash
# 1. 创建分支
git checkout -b feature/my-feature develop

# 2. 开发代码
# ... 编辑文件 ...

# 3. 提交
git add .
git commit -m "feat(scope): add new feature"

# 4. 推送
git push -u origin feature/my-feature

# 5. 创建 PR
# 在 GitHub/GitLab 上创建 PR 到 develop

# 6. 合并后清理
git checkout develop
git pull origin develop
git branch -d feature/my-feature
```

### 发布新版本

```bash
# 1. 创建发布分支
git checkout -b release/v1.1.0 develop

# 2. 更新版本号
# ... 编辑版本文件 ...
git commit -m "chore: bump version to 1.1.0"

# 3. 合并到 main
git checkout main
git merge --no-ff release/v1.1.0

# 4. 创建标签
git tag v1.1.0 -m "Release version 1.1.0"

# 5. 合并回 develop
git checkout develop
git merge --no-ff release/v1.1.0

# 6. 推送
git push origin main develop --tags

# 7. 删除发布分支
git branch -d release/v1.1.0
```

### 修复生产错误

```bash
# 1. 创建热修复分支
git checkout -b hotfix/v1.0.1-fix main

# 2. 修复问题
# ... 编辑文件 ...

# 3. 更新版本
git commit -m "fix: resolve production issue"
git commit -m "chore: bump version to 1.0.1"

# 4. 合并到 main
git checkout main
git merge --no-ff hotfix/v1.0.1-fix

# 5. 创建标签
git tag v1.0.1 -m "Hotfix version 1.0.1"

# 6. 合并到 develop
git checkout develop
git merge --no-ff hotfix/v1.0.1-fix

# 7. 推送
git push origin main develop --tags

# 8. 删除热修复分支
git branch -d hotfix/v1.0.1-fix
```

---

## 有用的别名

添加到 Git 配置以加速工作流：

```bash
# 配置别名
git config --global alias.co checkout
git config --global alias.br branch
git config --global alias.ci commit
git config --global alias.st status
git config --global alias.unstage 'restore --staged'
git config --global alias.last 'log -1 HEAD'
git config --global alias.visual 'log --graph --oneline --all --decorate'

# 使用别名
git co develop         # git checkout develop
git st                 # git status
git visual             # 查看分支图
```

---

## 常见问题速解

### 怎样撤销最后一次提交？

```bash
# 保留更改
git reset --soft HEAD~1

# 丢弃更改
git reset --hard HEAD~1
```

### 怎样修改最后一次提交？

```bash
git commit --amend -m "new message"
```

### 怎样恢复已删除的分支？

```bash
git reflog
git checkout -b <branch> <commit-hash>
```

### 怎样解决合并冲突？

```bash
# 1. 编辑冲突文件，移除 <<<, ===, >>> 标记
# 2. 暂存文件
git add .

# 3. 完成合并
git commit -m "Resolve merge conflict"
```

### 怎样推送所有分支和标签？

```bash
git push origin --all --tags
```

---

## 仓库统计

```bash
# 查看贡献者列表
git shortlog -sn

# 查看提交数统计
git rev-list --all --count

# 查看文件变化统计
git diff --stat HEAD~10

# 查看谁修改了这一行
git blame <file>
```

---

**记住**: 提交前始终运行测试和代码审核！🚀
