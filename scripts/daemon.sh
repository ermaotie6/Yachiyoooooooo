#!/bin/bash
# Yachiyo 进程守护 — 每分钟检查，挂了自动拉起
# 必须 source .env 获取真实凭证，不能用硬编码占位符！
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
LOG_DIR="$PROJECT_DIR/logs"
mkdir -p "$LOG_DIR"
DAEMON_LOG="$LOG_DIR/daemon.log"

log() { echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$DAEMON_LOG"; }

# 加载 .env 文件中的所有变量
if [ -f "$PROJECT_DIR/.env" ]; then
    set -a
    source "$PROJECT_DIR/.env"
    set +a
else
    log "❌ .env 文件不存在: $PROJECT_DIR/.env"
    exit 1
fi

# 验证关键变量
if [ -z "$OPENCLAW_GATEWAY_TOKEN" ] || [ -z "$DB_PASSWORD" ]; then
    log "❌ 关键环境变量缺失! OPENCLAW_GATEWAY_TOKEN=$([ -n "$OPENCLAW_GATEWAY_TOKEN" ] && echo 'SET' || echo 'MISSING') DB_PASSWORD=$([ -n "$DB_PASSWORD" ] && echo 'SET' || echo 'MISSING')"
    exit 1
fi

# ===== 后端守护 =====
check_backend() {
    if pgrep -f yachiyo_cpp > /dev/null 2>&1; then
        # 进程在，验证健康
        if curl -s --max-time 3 http://localhost:8080/api/v1/health > /dev/null 2>&1; then
            return 0  # 一切正常
        fi
        log "⚠️  Backend 进程存在但健康检查失败, 可能需要重启"
    fi

    log "⚠️  Backend 不在运行，重启中..."
    cd "$PROJECT_DIR"
    OPENCLAW_GATEWAY_TOKEN="$OPENCLAW_GATEWAY_TOKEN" \
    OPENCLAW_ENDPOINT="${OPENCLAW_ENDPOINT:-http://localhost:18789}" \
    OPENCLAW_MODEL="${OPENCLAW_MODEL:-openclaw/yachiyo}" \
    DB_PASSWORD="$DB_PASSWORD" \
    DB_HOST="${DB_HOST:-localhost}" \
    DB_NAME="${DB_NAME:-yachiyo}" \
    DB_USER="${DB_USER:-postgres}" \
    nohup ./backend/build/yachiyo_cpp --config-dir config --env dev \
        >> "$LOG_DIR/yachiyo.log" 2>&1 &
    local pid=$!
    disown $pid
    sleep 4
    if curl -s --max-time 3 http://localhost:8080/api/v1/health > /dev/null 2>&1; then
        log "✅ Backend 重启成功 (PID $pid)"
    else
        log "❌ Backend 重启后健康检查失败! (PID $pid)"
        tail -5 "$LOG_DIR/yachiyo.log" | while read line; do log "   $line"; done
    fi
}

# ===== 前端守护 =====
check_frontend() {
    if pgrep -f "vite.*5173" > /dev/null 2>&1; then
        if curl -s -o /dev/null --max-time 3 http://localhost:5173/ 2>&1; then
            return 0
        fi
    fi

    log "⚠️  Frontend 不在运行，重启中..."
    cd "$PROJECT_DIR/frontend"
    nohup npx vite --host 0.0.0.0 --port 5173 \
        >> "$LOG_DIR/vite.log" 2>&1 &
    local pid=$!
    disown $pid
    sleep 4
    if curl -s -o /dev/null --max-time 3 http://localhost:5173/ 2>&1; then
        log "✅ Frontend 重启成功 (PID $pid)"
    else
        log "❌ Frontend 重启失败! (PID $pid)"
    fi
}

check_backend
check_frontend
