#!/usr/bin/env bash
# ============================================================
# Yachiyo 部署 — 阶段 5: 构建启动
# 分步启动: PG → Redis → 后端 → 前端 → Nginx
# 用法: sudo bash scripts/deploy/stage5-build.sh
# ============================================================

set -euo pipefail

GREEN="\033[0;32m"; YELLOW="\033[1;33m"; RED="\033[0;31m"; BLUE="\033[0;34m"; CYAN="\033[0;36m"; NC="\033[0m"
info()  { echo -e "${BLUE}[阶段5]${NC} $*"; }
ok()    { echo -e "${GREEN}[✓]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
fail()  { echo -e "${RED}[✗]${NC} $*"; exit 1; }

INSTALL_DIR="${INSTALL_DIR:-/opt/yachiyo}"

echo -e "${CYAN}╔══════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  阶段 5: 构建启动                     ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════╝${NC}"
echo ""

cd "$INSTALL_DIR"

# ===== 前置检查 =====
if [ ! -f .env ]; then
    fail ".env 文件不存在，请先执行阶段 4"
fi

if [ ! -f docker-compose.yml ]; then
    fail "docker-compose.yml 不存在，请先执行阶段 3"
fi

# 检测 Docker Compose 命令
if docker compose version &>/dev/null; then
    DC="docker compose"
elif command -v docker-compose &>/dev/null; then
    DC="docker-compose"
else
    fail "Docker Compose 未安装，请先执行阶段 2"
fi

# ===== 步骤 1: 启动基础设施 =====
info "[1/4] 启动 PostgreSQL + Redis..."
$DC up -d postgres redis

info "等待 PostgreSQL 就绪..."
RETRIES=0
MAX=60
while [ $RETRIES -lt $MAX ]; do
    if $DC exec -T postgres pg_isready -U postgres &>/dev/null; then
        break
    fi
    sleep 2
    RETRIES=$((RETRIES+1))
    [ $((RETRIES % 10)) -eq 0 ] && info "  仍在等待 PostgreSQL... ($((RETRIES*2))s)"
done
if [ $RETRIES -ge $MAX ]; then
    warn "PostgreSQL 启动超时 (120s)"
    warn "查看日志: $DC logs --tail 30 postgres"
    warn "常见原因: 数据卷权限 / init.sql 错误 / 内存不足"
    $DC logs --tail 20 postgres 2>&1 || true
    fail "PostgreSQL 未就绪，无法继续"
fi
ok "PostgreSQL 已就绪"

info "等待 Redis 就绪..."
RETRIES=0
while [ $RETRIES -lt 15 ]; do
    if $DC exec -T redis redis-cli ping 2>/dev/null | grep -q PONG; then
        break
    fi
    sleep 2
    RETRIES=$((RETRIES+1))
done
ok "Redis 已就绪"

# ===== 步骤 2: 构建并启动后端 =====
info "[2/4] 构建 C++ 后端 (首次约 5~15 分钟)..."
info "  提示: 在另一个终端运行 '$DC logs -f backend' 可查看编译进度"

$DC up -d --build backend

info "等待后端启动..."
RETRIES=0
MAX=60
while [ $RETRIES -lt $MAX ]; do
    if curl -sf http://localhost:8080/api/v1/health &>/dev/null; then
        break
    fi
    sleep 5
    RETRIES=$((RETRIES+1))
    [ $((RETRIES % 6)) -eq 0 ] && info "仍在等待... ($((RETRIES*5))s)"
done
if [ $RETRIES -ge $MAX ]; then
    warn "后端启动超时 (5分钟)，可能仍在编译中"
    warn "检查日志: $DC logs --tail 50 backend"
    warn "脚本继续执行后续步骤..."
else
    ok "后端已启动"
fi

# ===== 步骤 3: 构建并启动前端 =====
info "[3/4] 构建 Vue 前端..."
$DC up -d --build frontend

RETRIES=0
while [ $RETRIES -lt 30 ]; do
    if curl -sf http://localhost:3000 &>/dev/null; then
        break
    fi
    sleep 3
    RETRIES=$((RETRIES+1))
done
if [ $RETRIES -lt 30 ]; then
    ok "前端已启动"
else
    warn "前端可能仍在构建中"
fi

# ===== 步骤 4: 启动 Nginx =====
info "[4/4] 启动 Nginx 反向代理..."
$DC up -d nginx
sleep 2

if curl -sf http://localhost &>/dev/null; then
    ok "Nginx 已启动"
else
    warn "Nginx 可能未就绪，但不影响继续"
fi

# ===== 显示状态 =====
echo ""
info "容器状态:"
$DC ps --format "table {{.Name}}\t{{.Status}}\t{{.Ports}}" 2>/dev/null || $DC ps

echo ""
ok "阶段 5 完成 ✓"
echo -e "  下一步: ${BLUE}sudo bash $INSTALL_DIR/scripts/deploy/stage6-verify.sh${NC}"
