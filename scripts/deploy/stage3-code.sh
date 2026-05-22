#!/usr/bin/env bash
# ============================================================
# Yachiyo 部署 — 阶段 3: 代码部署
# 克隆/更新项目代码到 /opt/yachiyo
# 用法: sudo bash scripts/deploy/stage3-code.sh
# 环境变量:
#   INSTALL_DIR  安装目录 (默认 /opt/yachiyo)
#   REPO_URL     仓库地址
#   BRANCH       分支 (默认 main)
# ============================================================

set -euo pipefail

GREEN="\033[0;32m"; YELLOW="\033[1;33m"; RED="\033[0;31m"; BLUE="\033[0;34m"; CYAN="\033[0;36m"; NC="\033[0m"
info()  { echo -e "${BLUE}[阶段3]${NC} $*"; }
ok()    { echo -e "${GREEN}[✓]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
fail()  { echo -e "${RED}[✗]${NC} $*"; exit 1; }

INSTALL_DIR="${INSTALL_DIR:-/opt/yachiyo}"
REPO_URL="${REPO_URL:-https://github.com/ermaotie6/yachiyoooooooo.git}"
BRANCH="${BRANCH:-main}"

echo -e "${CYAN}╔══════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  阶段 3: 代码部署                     ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════╝${NC}"
echo ""

REAL_USER="${SUDO_USER:-$USER}"

if [ -d "$INSTALL_DIR/.git" ]; then
    info "项目目录已存在，拉取最新代码..."
    cd "$INSTALL_DIR"
    git fetch origin
    git reset --hard "origin/$BRANCH"
    git clean -fd
    ok "代码更新完成: $(git log --oneline -1)"
else
    info "克隆项目到 $INSTALL_DIR ..."
    mkdir -p "$INSTALL_DIR"
    chown "$REAL_USER:$REAL_USER" "$INSTALL_DIR"
    git clone --branch "$BRANCH" "$REPO_URL" "$INSTALL_DIR"
    chown -R "$REAL_USER:$REAL_USER" "$INSTALL_DIR"
    cd "$INSTALL_DIR"
    ok "项目克隆完成: $(git log --oneline -1)"
fi

# 确保脚本有执行权限
chmod +x "$INSTALL_DIR/scripts/"*.sh 2>/dev/null || true
chmod +x "$INSTALL_DIR/scripts/deploy/"*.sh 2>/dev/null || true

# 展示项目结构
info "项目结构:"
echo "  $INSTALL_DIR/"
echo "  ├── backend/         C++ 后端 (Crow + PostgreSQL)"
echo "  ├── frontend/        Vue 3 前端 (Vite + PixiJS + Live2D)"
echo "  ├── database/        SQL 初始化脚本"
echo "  ├── config/          全局配置文件"
echo "  ├── resources/       Live2D 模型 + 头像配置"
echo "  ├── nginx.conf       Nginx 反向代理配置"
echo "  ├── docker-compose.yml"
echo "  └── scripts/"
echo "      └── deploy/      分阶段部署脚本 (stage1~7)"

echo ""
ok "阶段 3 完成 ✓"
echo -e "  下一步: ${BLUE}sudo bash $INSTALL_DIR/scripts/deploy/stage4-config.sh${NC}"
