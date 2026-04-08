#!/usr/bin/env bash
# ============================================================
# Yachiyo 部署 — 阶段 6: 验证测试
# 自动化健康检查：容器、API、前端、Nginx、数据库、Redis、WebSocket
# 用法: sudo bash scripts/deploy/stage6-verify.sh
# ============================================================

set -euo pipefail

GREEN="\033[0;32m"; YELLOW="\033[1;33m"; RED="\033[0;31m"; BLUE="\033[0;34m"; CYAN="\033[0;36m"; NC="\033[0m"
info()  { echo -e "${BLUE}[阶段6]${NC} $*"; }
ok()    { echo -e "${GREEN}[✓]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
fail()  { echo -e "${RED}[✗]${NC} $*"; }

INSTALL_DIR="${INSTALL_DIR:-/opt/yachiyo}"
PASS=0
FAIL=0
WARN=0

echo -e "${CYAN}╔══════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  阶段 6: 验证测试                     ║${NC}"
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

# ===== 测试 1: 容器状态 =====
echo -e "${BLUE}[测试 1] 容器运行状态${NC}"
EXPECTED_CONTAINERS="postgres redis backend frontend nginx"
for SVC in $EXPECTED_CONTAINERS; do
    STATUS=$($DC ps --format "{{.Status}}" "$SVC" 2>/dev/null || echo "not found")
    if echo "$STATUS" | grep -qi "up"; then
        if echo "$STATUS" | grep -qi "healthy"; then
            ok "$SVC: 运行中 (健康)"
        else
            ok "$SVC: 运行中"
        fi
        PASS=$((PASS+1))
    else
        fail "$SVC: $STATUS"
        FAIL=$((FAIL+1))
    fi
done

# ===== 测试 2: 后端 API =====
echo ""
echo -e "${BLUE}[测试 2] 后端 API 健康检查${NC}"
HEALTH_RESP=$(curl -sf http://localhost:8080/api/v1/health 2>/dev/null || echo "")
if [ -n "$HEALTH_RESP" ]; then
    ok "GET /api/v1/health → 200 OK"
    echo "  响应: $(echo "$HEALTH_RESP" | head -c 200)"
    echo ""
    PASS=$((PASS+1))
else
    fail "GET /api/v1/health 无响应"
    FAIL=$((FAIL+1))
fi

# ===== 测试 3: 前端 =====
echo ""
echo -e "${BLUE}[测试 3] 前端服务${NC}"
HTTP_CODE=$(curl -so /dev/null -w "%{http_code}" http://localhost:3000 2>/dev/null || echo "000")
if [ "$HTTP_CODE" = "200" ]; then
    ok "GET localhost:3000 → 200 OK"
    PASS=$((PASS+1))
else
    fail "GET localhost:3000 → $HTTP_CODE"
    FAIL=$((FAIL+1))
fi

# ===== 测试 4: Nginx 反向代理 =====
echo ""
echo -e "${BLUE}[测试 4] Nginx 反向代理${NC}"

# 4a: 前端代理
HTTP_CODE=$(curl -so /dev/null -w "%{http_code}" http://localhost/ 2>/dev/null || echo "000")
if [ "$HTTP_CODE" = "200" ] || [ "$HTTP_CODE" = "301" ]; then
    ok "GET localhost/ → $HTTP_CODE (前端代理正常)"
    PASS=$((PASS+1))
else
    fail "GET localhost/ → $HTTP_CODE"
    FAIL=$((FAIL+1))
fi

# 4b: API 代理
HTTP_CODE=$(curl -so /dev/null -w "%{http_code}" http://localhost/api/v1/health 2>/dev/null || echo "000")
if [ "$HTTP_CODE" = "200" ]; then
    ok "GET localhost/api/v1/health → 200 (API 代理正常)"
    PASS=$((PASS+1))
else
    warn "GET localhost/api/v1/health → $HTTP_CODE (可能被限流)"
    WARN=$((WARN+1))
fi

# ===== 测试 5: 数据库 =====
echo ""
echo -e "${BLUE}[测试 5] 数据库连接${NC}"
DB_TABLES=$($DC exec -T postgres psql -U postgres -d yachiyo -t -c \
    "SELECT count(*) FROM information_schema.tables WHERE table_schema='public'" 2>/dev/null || echo "0")
DB_TABLES=$(echo "$DB_TABLES" | tr -d ' ')
if [ "$DB_TABLES" -gt 5 ] 2>/dev/null; then
    ok "PostgreSQL 已连接，检测到 $DB_TABLES 张表"
    PASS=$((PASS+1))
else
    warn "PostgreSQL 表数量异常: $DB_TABLES"
    WARN=$((WARN+1))
fi

# ===== 测试 6: Redis =====
echo ""
echo -e "${BLUE}[测试 6] Redis 缓存${NC}"
REDIS_PONG=$($DC exec -T redis redis-cli ping 2>/dev/null || echo "")
if echo "$REDIS_PONG" | grep -q "PONG"; then
    ok "Redis PING → PONG"
    PASS=$((PASS+1))
else
    fail "Redis 无响应"
    FAIL=$((FAIL+1))
fi

# ===== 测试 7: WebSocket =====
echo ""
echo -e "${BLUE}[测试 7] WebSocket 端口${NC}"
# Crow WebSocket 不会响应普通 HTTP GET，尝试容器内检测
if $DC exec -T backend sh -c "ss -tlnp | grep -q 9001" 2>/dev/null; then
    ok "WebSocket 端口 9001 在容器内监听"
    PASS=$((PASS+1))
elif curl -sf -o /dev/null --connect-timeout 3 http://localhost:9001/ 2>/dev/null; then
    ok "WebSocket 端口 9001 可达"
    PASS=$((PASS+1))
else
    warn "WebSocket 端口 9001 不可达 (可能正常，Crow WS 不响应普通 HTTP)"
    WARN=$((WARN+1))
fi

# ===== 汇总 =====
echo ""
echo -e "${CYAN}════════════════════════════════════════${NC}"
echo -e "  通过: ${GREEN}$PASS${NC}  |  警告: ${YELLOW}$WARN${NC}  |  失败: ${RED}$FAIL${NC}"
echo -e "${CYAN}════════════════════════════════════════${NC}"

if [ $FAIL -eq 0 ]; then
    echo ""
    ok "所有关键测试通过 ✓"
    echo ""
    echo -e "${GREEN}╔══════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║       🎀 Yachiyo 部署成功！                  ║${NC}"
    echo -e "${GREEN}╠══════════════════════════════════════════════╣${NC}"
    echo -e "${GREEN}║  前端:      http://localhost                 ║${NC}"
    echo -e "${GREEN}║  后端 API:  http://localhost/api/v1/health   ║${NC}"
    echo -e "${GREEN}║  WebSocket: ws://localhost/ws                ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "  下一步 (可选): ${BLUE}sudo bash $INSTALL_DIR/scripts/deploy/stage7-secure.sh${NC}  ← 启用 HTTPS"
else
    echo ""
    fail "有 $FAIL 项测试失败，请检查日志:"
    echo "  $DC logs backend"
    echo "  $DC logs postgres"
    echo "  $DC logs frontend"
    echo "  $DC logs nginx"
fi

exit $FAIL
