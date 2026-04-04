# 🔄 Yachiyo 版本控制与发布管理指南

**版本**: 1.0  
**更新时间**: 2026-04-02  
**适用范围**: Git 工作流、版本管理、发布流程

---

## 📑 目录

1. [Git 工作流](#git-工作流)
2. [版本号管理](#版本号管理)
3. [发布流程](#发布流程)
4. [变更日志](#变更日志)
5. [标签和里程碑](#标签和里程碑)
6. [回滚指南](#回滚指南)

---

## Git 工作流

### 分支策略

```
main (生产分支)
  ↑
  ├─── v1.0.0 (发布标签)
  │
  ├─── 从 release/1.0 快进合并
  │
release/1.0 (发布准备分支)
  ↑
  ├─── 从 develop 创建
  │
  ├─── 修复发布前问题
  │
develop (开发主分支)
  ↑
  ├─── feature/user-auth
  ├─── feature/ai-response
  ├─── feature/voice-synthesis
  └─── bugfix/performance-issue
```

### 分支命名规范

```
主分支:
  main              - 生产环境稳定版本
  develop           - 集成分支，所有功能合并处

功能分支:
  feature/*         - 新功能开发
  feature/auth-2fa  - 2FA 认证功能
  feature/cache-opt - 缓存优化
  
修复分支:
  bugfix/*          - Bug 修复
  bugfix/sql-inject - SQL 注入修复
  hotfix/*          - 生产环保急修复
  
优化分支:
  refactor/*        - 代码重构
  chore/*           - 构建、依赖更新
  docs/*            - 文档更新

发布分支:
  release/*         - 发布准备
  release/1.0       - 1.0 版本准备
```

### 提交信息规范

遵循 Conventional Commits:

```
<type>[optional scope]: <description>

[optional body]

[optional footer]
```

**Type 列表**:
```
feat:      新功能
fix:       Bug 修复
docs:      文档变更
style:      代码风格 (无逻辑变更)
refactor:  代码重构
perf:      性能优化
test:      测试代码
ci:        CI/CD 配置
chore:     依赖更新、工具更新
```

**提交示例**:

```bash
# 新功能
git commit -m "feat(auth): implement 2FA authentication with TOTP"

# Bug 修复
git commit -m "fix(cache): resolve Redis connection pool exhaustion
- Fixed connection leak in cache manager
- Added connection timeout handling
Fixes #123"

# 性能优化
git commit -m "perf(database): add composite index on messages table

Previously queries on user_id + timestamp took 500ms.
After indexing, queries take 10ms.
Benchmark: 50x improvement

Closes #456"

# 破坏性变更
git commit -m "feat!: redesign API response format

BREAKING CHANGE: API response structure changed from:
{status: 200, data: {...}}
to:
{success: true, payload: {...}}

Migration guide in docs/migration-v1-to-v2.md"
```

### 工作流示例

```bash
# 1. 创建功能分支
git checkout -b feature/2fa-authentication develop

# 2. 开发功能
# ... 编写代码 ...
git add .
git commit -m "feat(auth): add TOTP 2FA support"
git commit -m "feat(auth): implement backup codes"

# 3. 定期从 develop 同步最新代码
git fetch origin develop
git rebase origin/develop

# 4. 创建 Pull Request
git push origin feature/2fa-authentication
# 在 GitHub 创建 PR

# 5. 代码审查和 CI/CD 检查
# ... 等待审查和自动化测试 ...

# 6. 处理反馈（如需要）
# ... 进行修改 ...
git commit -m "fix(auth): handle edge case in TOTP validation"
git push origin feature/2fa-authentication

# 7. 合并到 develop（通过 PR 或手动）
git checkout develop
git pull origin develop
git merge --no-ff feature/2fa-authentication
git push origin develop

# 8. 删除功能分支
git push origin --delete feature/2fa-authentication
git branch -d feature/2fa-authentication
```

---

## 版本号管理

### 语义版本化 (Semantic Versioning)

遵循 MAJOR.MINOR.PATCH 格式:

```
v1.2.3
  ↑ ↑ ↑
  │ │ └─ PATCH: Bug 修复, 后向兼容
  │ └─── MINOR: 新功能, 后向兼容
  └───── MAJOR: 破坏性变更, 不后向兼容
```

**版本示例**:

```
v0.1.0    - 初始开发版本
v0.2.0    - 添加新功能
v0.2.1    - Bug 修复
v1.0.0    - 第一个稳定版本 🎉
v1.1.0    - 新功能 (后向兼容)
v1.1.1    - Bug 修复
v1.2.0    - 更多新功能
v2.0.0    - 主版本升级 (破坏性)
```

### 版本维护计划

```
当前版本:     v1.0.0 (在开发)
稳定版本:     v1.0.0 (2026-04-09 发布)
长期支持版:   v1.0.x (2026-04-09 ~ 2027-04-09)
下一版本:     v2.0.0 (2027 年计划)

支持时间线:
┌─────────────────────────────────────┐
│ v1.0.x (LTS)                        │
│ 2026-04-09 ─────────────2027-04-09  │
│     ↓                      ↓         │
│  稳定期                 支持结束期    │
└─────────────────────────────────────┘
```

---

## 发布流程

### 完整发布清单

```bash
#!/bin/bash
# release.sh - 完整发布流程

set -e

VERSION="${1:-1.0.0}"

echo "=== Yachiyo 发布流程 v$VERSION ==="

# 1. 验证前置条件
echo "【1】验证前置条件..."
if [ -z "$(git status --porcelain)" ]; then
    echo "✅ 工作目录干净"
else
    echo "❌ 工作目录有未提交的修改"
    git status
    exit 1
fi

# 2. 创建发布分支
echo ""
echo "【2】创建发布分支..."
git checkout -b release/$VERSION develop
echo "✅ 创建 release/$VERSION 分支"

# 3. 更新版本号
echo ""
echo "【3】更新版本号..."
sed -i "s/VERSION = .*/VERSION = $VERSION/" CMakeLists.txt
sed -i "s/\"version\": .*/\"version\": \"$VERSION\",/" package.json
git add CMakeLists.txt package.json
git commit -m "chore: bump version to $VERSION"
echo "✅ 版本号已更新"

# 4. 更新 CHANGELOG
echo ""
echo "【4】生成 CHANGELOG..."
# 使用 conventional-changelog-cli
# npm install -g conventional-changelog-cli
conventional-changelog -p angular -i CHANGELOG.md -s -r 0
git add CHANGELOG.md
git commit -m "docs: update CHANGELOG for $VERSION"
echo "✅ CHANGELOG 已更新"

# 5. 运行最终测试
echo ""
echo "【5】运行最终测试..."
npm run test:full || {
    echo "❌ 测试失败"
    exit 1
}
echo "✅ 所有测试通过"

# 6. 构建发布版本
echo ""
echo "【6】构建发布版本..."
npm run build:release
echo "✅ 构建完成"

# 7. 创建 PR
echo ""
echo "【7】创建发布 PR..."
echo "请手动创建 PR: release/$VERSION -> main"
echo "PR 标题: Release v$VERSION"
echo "PR 描述:"
echo "  This PR merges $VERSION into main for production release."
echo "  - Version: $VERSION"
echo "  - Release Date: $(date)"
echo "  - Branch: release/$VERSION"
echo ""
read -p "按 Enter 继续..."

# 8. 等待 PR 审查和 CI/CD
echo ""
echo "【8】等待审查和 CI/CD..."
echo "请在 GitHub 等待:"
echo "  - 代码审查批准"
echo "  - CI/CD 测试通过"
echo ""
read -p "PR 已批准? (y/n) " approved
if [ "$approved" != "y" ]; then
    echo "❌ PR 未被批准，发布中止"
    exit 1
fi

# 9. 合并到 main
echo ""
echo "【9】合并到 main..."
git checkout main
git pull origin main
git merge --no-ff release/$VERSION -m "Merge release/$VERSION into main"
git push origin main
echo "✅ 已合并到 main"

# 10. 创建发布标签
echo ""
echo "【10】创建发布标签..."
git tag -a v$VERSION -m "Release version $VERSION

Features:
  - $(grep -A 20 '## \[' CHANGELOG.md | head -20 | tail -19)

Contributors:
  - $(git log v$((VERSION-1)).0..HEAD --format='%an' | sort | uniq | tr '\n' ', ')"
git push origin v$VERSION
echo "✅ 标签已创建: v$VERSION"

# 11. 创建 GitHub Release
echo ""
echo "【11】创建 GitHub Release..."
RELEASE_NOTES=$(sed -n "/## \[$VERSION\]/,/## \[/p" CHANGELOG.md | head -n -1)
gh release create v$VERSION \
  --title "Release v$VERSION" \
  --notes "$RELEASE_NOTES" \
  ./dist/* || echo "⚠️ 请手动在 GitHub 创建 Release"

# 12. 合并回 develop
echo ""
echo "【12】合并回 develop..."
git checkout develop
git pull origin develop
git merge --no-ff main -m "Merge main back into develop after $VERSION release"
git push origin develop
echo "✅ 已合并回 develop"

# 13. 删除发布分支
echo ""
echo "【13】清理发布分支..."
git push origin --delete release/$VERSION
git branch -d release/$VERSION
echo "✅ 发布分支已删除"

# 14. 通知团队
echo ""
echo "【14】通知团队..."
echo "🎉 版本 v$VERSION 发布成功！"
echo ""
echo "通知内容:"
echo "  - 版本: v$VERSION"
echo "  - GitHub Release: https://github.com/yachiyo/releases/tag/v$VERSION"
echo "  - CHANGELOG: https://github.com/yachiyo/blob/main/CHANGELOG.md"
echo ""
echo "发布完成！"
```

---

## 变更日志

### CHANGELOG.md 格式

```markdown
# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- New features under development

### Changed
- Ongoing changes

### Deprecated
- Features to be removed in future versions

### Removed
- Features removed in this release

### Fixed
- Bug fixes in this release

### Security
- Security-related changes

## [1.0.0] - 2026-04-09

### Added
- 2FA (TOTP) authentication support
- Rate limiting (global 100 req/s, per-user 10 req/s)
- API Key management with HashiCorp Vault
- Prometheus metrics collection
- Grafana dashboards
- ELK Stack logging
- Docker Compose development environment
- Kubernetes deployment manifests
- GitHub Actions CI/CD pipeline
- Blue-green deployment support
- Health checks and liveness probes
- Automatic horizontal scaling (HPA)
- Redis connection pooling
- PostgreSQL connection pooling
- Multi-language support (English, Chinese)

### Changed
- Refactored database layer for better performance
- Improved error handling and logging
- Enhanced security with input validation
- Updated dependencies to latest versions
- Optimized Docker image sizes

### Fixed
- Fixed WebSocket connection stability issues
- Fixed cache invalidation race conditions
- Fixed database connection leaks
- Fixed JWT token expiration handling
- Fixed CORS configuration for production

### Security
- Added SQL injection prevention
- Implemented CSRF protection
- Added rate limiting
- Implemented request validation
- Added audit logging

## [0.2.0] - 2026-02-15

### Added
- Live2D animation support
- Voice synthesis with GPT-SoVITS
- OpenAI GPT-3.5 integration
- Redis caching layer
- JWT authentication

### Fixed
- Fixed WebSocket disconnection issues
- Improved error messages

## [0.1.0] - 2026-01-20

### Added
- Initial project setup
- Basic chat functionality
- User authentication
- Database integration
- Frontend with Vue 3
- Backend with C++20 and Crow Framework
```

### 自动生成 CHANGELOG

```bash
# 使用 conventional-changelog-cli
npm install -g conventional-changelog-cli

# 生成完整 CHANGELOG
conventional-changelog -p angular -i CHANGELOG.md -s

# 生成特定版本
conventional-changelog -p angular -i CHANGELOG.md -s -r 1

# 预览 CHANGELOG
conventional-changelog -p angular -u
```

---

## 标签和里程碑

### Git 标签

```bash
# 创建带注解的标签（推荐）
git tag -a v1.0.0 -m "Release version 1.0.0"

# 创建轻量标签
git tag v1.0.0

# 显示标签信息
git show v1.0.0

# 推送标签
git push origin v1.0.0
git push origin --tags  # 推送所有标签

# 删除标签
git tag -d v1.0.0
git push origin --delete v1.0.0
```

### GitHub 里程碑

```bash
# 通过 GitHub CLI 创建里程碑
gh milestone create "v1.0.0" --description "First stable release"

# 列出所有里程碑
gh milestone list

# 查看里程碑进度
gh milestone view "v1.0.0"
```

### GitHub Release

```bash
# 创建发布
gh release create v1.0.0 \
  --title "Release 1.0.0" \
  --notes "First stable release" \
  --draft  # 创建草稿

# 发布草稿版本
gh release edit v1.0.0 --draft=false

# 上传发布文件
gh release upload v1.0.0 ./dist/*.tar.gz

# 列出所有发布
gh release list
```

---

## 回滚指南

### 本地回滚

```bash
# 撤销未提交的更改
git restore <file>              # 特定文件
git restore .                   # 所有文件

# 撤销已提交的更改
git revert <commit>             # 创建新提交来撤销
git reset --soft <commit>       # 保留文件更改
git reset --hard <commit>       # 完全回到该提交

# 恢复已删除的分支
git reflog
git checkout -b <branch> <commit_hash>
```

### 生产回滚

```bash
#!/bin/bash
# production_rollback.sh - 生产环境回滚

set -e

CURRENT_VERSION="v1.0.0"
ROLLBACK_VERSION="v0.9.0"

echo "⚠️ 生产环境回滚流程"
echo "当前版本: $CURRENT_VERSION"
echo "回滚版本: $ROLLBACK_VERSION"
echo ""
read -p "确认回滚? (yes/no) " confirm
[ "$confirm" = "yes" ] || exit 1

# 1. 通知团队
echo "【1】通知团队..."
echo "发送告警: 生产环境开始回滚"

# 2. 停止新部署
echo "【2】停止新部署..."
kubectl set deployment.apps/yachiyo-backend replicas=0 -n yachiyo || true

# 3. 检查备份
echo "【3】检查数据库备份..."
ls -lh backups/yachiyo_db_*.sql.gz | tail -1

# 4. 恢复旧版本
echo "【4】恢复到 $ROLLBACK_VERSION..."
git checkout $ROLLBACK_VERSION
docker-compose down
docker-compose up -d

# 5. 验证服务
echo "【5】验证服务..."
sleep 30
curl http://localhost:8080/api/health || {
    echo "❌ 健康检查失败"
    exit 1
}

# 6. 恢复数据库（如需要）
echo "【6】恢复数据库..."
# psql -U yachiyo_user -d yachiyo_db < backups/latest_backup.sql

# 7. 验证功能
echo "【7】验证功能..."
npm run test:smoke

# 8. 更新状态
echo "【8】更新状态页面..."
echo "已回滚到 $ROLLBACK_VERSION"

echo "✅ 回滚完成"
```

### 部分功能回滚

```bash
#!/bin/bash
# feature_rollback.sh - 特定功能回滚

set -e

FEATURE="2fa-authentication"
FEATURE_FLAG="FEATURE_2FA_ENABLED"

echo "回滚功能: $FEATURE"

# 方法 1: 禁用功能标志
echo "禁用功能标志..."
kubectl set env deployment/yachiyo-backend \
  $FEATURE_FLAG=false -n yachiyo

# 方法 2: 恢复特定模块
echo "恢复特定模块..."
git checkout <previous_commit> -- src/auth/
cd backend && cmake --build build -- -j$(nproc)

# 方法 3: 蓝绿切换
echo "切换到前一版本..."
kubectl patch service yachiyo-backend -n yachiyo \
  -p "{\"spec\":{\"selector\":{\"version\":\"v1.0.0\"}}}"

echo "✅ 功能已回滚"
```

---

## 常见情景

### 情景 1: 发现生产 Bug

```bash
# 1. 创建快速修复分支
git checkout -b hotfix/sql-injection-fix main

# 2. 修复 Bug
# ... 编写修复 ...
git add .
git commit -m "fix(security): prevent SQL injection in user query"

# 3. 测试修复
npm run test

# 4. 立即发布到生产
git checkout main
git merge --no-ff hotfix/sql-injection-fix
git tag -a v1.0.1 -m "Hotfix: SQL injection prevention"
git push origin main v1.0.1

# 5. 回合到 develop
git checkout develop
git merge --no-ff hotfix/sql-injection-fix
git push origin develop

# 6. 清理
git branch -d hotfix/sql-injection-fix
```

### 情景 2: 需要快速回滚

```bash
# 1. 立即应急通知
slack-notify "🚨 生产问题，准备回滚"

# 2. 快速检查日志
kubectl logs -n yachiyo -l app=yachiyo-backend --tail=100

# 3. 执行回滚
git checkout v1.0.0
docker-compose down
docker-compose up -d

# 4. 验证
curl http://localhost:8080/api/health

# 5. 后续分析
# 稍后分析根本原因
```

### 情景 3: 协作开发

```bash
# 开发者 A:
git checkout -b feature/frontend-redesign develop
# ... 开发 ...
git push origin feature/frontend-redesign
# 创建 PR

# 开发者 B:
git checkout -b feature/backend-optimization develop
# ... 开发 ...
git push origin feature/backend-optimization
# 创建 PR

# 集成管理员:
git checkout develop
git pull origin develop
git merge --no-ff feature/frontend-redesign
git merge --no-ff feature/backend-optimization
# 如果有冲突，解决冲突
git push origin develop
```

---

**版本管理完成！** 🔄

---

*最后更新: 2026-04-02*  
*下一步: 创建发布检查清单*
