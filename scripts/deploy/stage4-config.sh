#!/usr/bin/env bash
# ============================================================
# Yachiyo 部署 — 阶段 4: 密钥配置
# 交互式生成 .env 文件，自动生成 JWT 密钥
# 用法: sudo bash scripts/deploy/stage4-config.sh
# ============================================================

set -euo pipefail

GREEN="\033[0;32m"; YELLOW="\033[1;33m"; RED="\033[0;31m"; BLUE="\033[0;34m"; CYAN="\033[0;36m"; NC="\033[0m"
info()  { echo -e "${BLUE}[阶段4]${NC} $*"; }
ok()    { echo -e "${GREEN}[✓]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
fail()  { echo -e "${RED}[✗]${NC} $*"; exit 1; }

INSTALL_DIR="${INSTALL_DIR:-/opt/yachiyo}"
ENV_FILE="$INSTALL_DIR/.env"

echo -e "${CYAN}╔══════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  阶段 4: 密钥配置                     ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════╝${NC}"
echo ""

if [ ! -d "$INSTALL_DIR" ]; then
    fail "项目目录 $INSTALL_DIR 不存在，请先执行阶段 3"
fi

# ===== 处理已有 .env =====
if [ -f "$ENV_FILE" ]; then
    warn ".env 已存在: $ENV_FILE"
    read -p "  覆盖重新配置? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        ok "保留现有配置，跳过"
        exit 0
    fi
    BACKUP="${ENV_FILE}.backup.$(date +%Y%m%d_%H%M%S)"
    cp "$ENV_FILE" "$BACKUP"
    ok "已备份到: $BACKUP"
fi

# ===== 自动生成 JWT 密钥 =====
JWT_SECRET=$(openssl rand -hex 32)
ok "JWT 密钥已自动生成"

# ===== 交互式填写 =====
echo ""
echo -e "${BLUE}请输入以下配置（直接回车跳过，稍后可编辑 .env 补充）:${NC}"
echo ""

# 数据库
read -p "  数据库密码 [postgres]: " DB_PASS
DB_PASS="${DB_PASS:-postgres}"

echo ""

# OpenClaw Gateway（必选 — AI 核心）
echo -e "${BLUE}OpenClaw Gateway (必选 — AI 对话 + 审查 + 翻译):${NC}"
read -p "  Gateway 端点 [http://host.docker.internal:8100]: " OC_ENDPOINT
OC_ENDPOINT="${OC_ENDPOINT:-http://host.docker.internal:8100}"

read -p "  认证令牌: " OC_TOKEN

read -p "  模型名称 [deepseek/deepseek-v4-flash]: " OC_MODEL
OC_MODEL="${OC_MODEL:-deepseek/deepseek-v4-flash}"

echo ""

# GPT-SoVITS（可选）
echo -e "${BLUE}GPT-SoVITS 语音合成 (可选，不部署则留空):${NC}"
read -p "  TTS 服务端点 [http://localhost:5000]: " SOVITS_ENDPOINT
SOVITS_ENDPOINT="${SOVITS_ENDPOINT:-http://localhost:5000}"

# ===== 域名 (用于 SSL 证书自动配置) =====
echo ""
echo -e "${BLUE}域名 (可选 — 用于 HTTPS/SSL 自动配置):${NC}"
read -p "  HTTPS 域名 [留空跳过]: " DOMAIN
echo ""

# ===== 生成 .env =====
cat > "$ENV_FILE" <<ENVEOF
# ============================================================
# Yachiyo 环境变量配置
# 由 stage4-config.sh 于 $(date '+%Y-%m-%d %H:%M:%S') 生成
# ============================================================

# ===== JWT 签名密钥 (已自动生成) =====
JWT_SECRET_KEY=${JWT_SECRET}

# ===== 数据库 =====
DB_PASSWORD=${DB_PASS}

# ===== OpenClaw Gateway (必选 — AI 核心: 对话 + 审查 + 翻译) =====
OPENCLAW_ENDPOINT=${OC_ENDPOINT}
OPENCLAW_GATEWAY_TOKEN=${OC_TOKEN}
OPENCLAW_MODEL=${OC_MODEL}

# ===== GPT-SoVITS 语音合成 (可选) =====
SOVITS_ENDPOINT=${SOVITS_ENDPOINT}

# ===== 域名 (用于 HTTPS/SSL) =====
YACHIYO_DOMAIN=${DOMAIN}

# ===== 部署模式 =====
ENVIRONMENT=production
ENVEOF

chmod 600 "$ENV_FILE"
ok ".env 已生成: $ENV_FILE"

# ===== 验证必填项 =====
echo ""
MISSING=0
[ -z "$OC_TOKEN" ] && { warn "OPENCLAW_GATEWAY_TOKEN 未填写 — AI 功能将不可用"; MISSING=$((MISSING+1)); }
[ "$DB_PASS" = "postgres" ] && { warn "数据库密码仍为默认值 'postgres'"; MISSING=$((MISSING+1)); }

if [ $MISSING -gt 0 ]; then
    echo ""
    warn "有 $MISSING 项未填写，稍后请编辑: nano $ENV_FILE"
fi

echo ""
ok "阶段 4 完成"
echo -e "  下一步: sudo bash $INSTALL_DIR/scripts/deploy/stage5-build.sh"
