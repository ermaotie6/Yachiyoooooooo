#!/usr/bin/env bash
# ============================================================
# Yachiyo 更新部署脚本
# 拉取最新代码 → 重建服务 → 健康检查
# 用法: sudo bash scripts/update.sh [backend|frontend|all]
# ============================================================

set -euo pipefail

GREEN="\033[0;32m"; YELLOW="\033[1;33m"; RED="\033[0;31m"; BLUE="\033[0;34m"; CYAN="\033[0;36m"; NC="\033[0m"
info()  { echo -e "${BLUE}[更新]${NC} $*"; }
ok()    { echo -e "${GREEN}[✓]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
fail()  { echo -e "${RED}[✗]${NC} $*"; }

# 定位项目目录 (脚本在 scripts/ 下)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

TARGET="${1:-all}"

echo -e "${CYAN}╔══════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  🎀 Yachiyo 更新部署                  ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════╝${NC}"
echo ""

cd "$INSTALL_DIR"

# 检测 Docker Compose
if docker compose version &>/dev/null; then
    DC="docker compose"
elif command -v docker-compose &>/dev/null; then
    DC="docker-compose"
else
    fail "Docker Compose 不可用"
    exit 1
fi

# ===== 拉取代码 =====
info "拉取最新代码..."
git pull origin main
ok "代码更新: $(git log --oneline -1)"

# ===== 重建服务 =====
case "$TARGET" in
    backend)
        info "重建后端..."
        $DC up -d --build backend
        ;;
    frontend)
        info "重建前端..."
        $DC up -d --build frontend
        ;;
    all|*)
        info "重建后端 + 前端..."
        $DC up -d --build backend frontend
        ;;
esac

# ===== 健康检查 =====
info "等待后端启动..."
RETRIES=0
MAX=24
while [ $RETRIES -lt $MAX ]; do
    if curl -sf http://localhost:8080/api/v1/health > /dev/null 2>&1; then
        break
    fi
    sleep 5
    RETRIES=$((RETRIES+1))
done

if [ $RETRIES -lt $MAX ]; then
    ok "更新成功！后端健康检查通过"
else
    warn "后端健康检查超时 (2分钟)"
    warn "检查日志: $DC logs --tail 50 backend"
fi

# 显示状态
echo ""
info "容器状态:"
$DC ps --format "table {{.Name}}\t{{.Status}}" 2>/dev/null || $DC ps
