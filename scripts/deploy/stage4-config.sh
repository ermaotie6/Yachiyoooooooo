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

# ===== 检查项目目录 =====
if [ ! -d "$INSTALL_DIR" ]; then
    fail "项目目录 $INSTALL_DIR 不存在，请先执行阶段 3"
fi

# ===== 处理已有 .env =====
if [ -f "$ENV_FILE" ]; then
    warn ".env 文件已存在: $ENV_FILE"
    read -p "  是否覆盖重新配置? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        ok "保留现有配置，跳过"
        echo -e "  如需修改: ${YELLOW}nano $ENV_FILE${NC}"
        exit 0
    fi
    BACKUP="${ENV_FILE}.backup.$(date +%Y%m%d_%H%M%S)"
    cp "$ENV_FILE" "$BACKUP"
    ok "已备份到: $BACKUP"
fi

# ===== 自动生成 JWT 密钥 =====
JWT_SECRET=$(openssl rand -hex 32)
ok "JWT 密钥已自动生成 (64 字符随机字符串)"

# ===== 交互式填写 =====
echo ""
echo -e "${BLUE}请输入以下 API 密钥（直接回车可跳过，稍后编辑 .env 补充）:${NC}"
echo ""

read -p "  DeepSeek API Key (sk-开头): " DEEPSEEK_KEY
DEEPSEEK_KEY="${DEEPSEEK_KEY:-}"

read -p "  百度翻译 APPID (纯数字): " BAIDU_APPID
BAIDU_APPID="${BAIDU_APPID:-}"

read -p "  百度翻译密钥 (字母数字): " BAIDU_SECRET
BAIDU_SECRET="${BAIDU_SECRET:-}"

read -p "  数据库密码 (默认 postgres): " DB_PASS
DB_PASS="${DB_PASS:-postgres}"

# ===== 生成 .env =====
cat > "$ENV_FILE" <<ENVEOF
# ============================================================
# Yachiyo 环境变量配置
# 由 stage4-config.sh 于 $(date '+%Y-%m-%d %H:%M:%S') 生成
# ============================================================

# ===== JWT 签名密钥 (已自动生成) =====
JWT_SECRET_KEY=${JWT_SECRET}

# ===== DeepSeek API (AI 对话 + 内容审核) =====
DEEPSEEK_API_KEY=${DEEPSEEK_KEY}

# ===== 百度翻译 =====
# 获取地址: https://fanyi-api.baidu.com/manage/developer
BAIDU_TRANSLATE_APPID=${BAIDU_APPID}
BAIDU_TRANSLATE_API_KEY=${BAIDU_SECRET}

# ===== 数据库 =====
DATABASE_PASSWORD=${DB_PASS}

# ===== OpenClaw (可选，不部署则留空) =====
OPENCLAW_ENDPOINT=
OPENCLAW_API_KEY=

# ===== GPT-SoVITS 语音合成 (可选) =====
SOVITS_API_KEY=

# ===== 部署模式 =====
ENVIRONMENT=production
ENVEOF

chmod 600 "$ENV_FILE"
ok ".env 文件已生成: $ENV_FILE"

# ===== 验证必填项 =====
MISSING=0
[ -z "$DEEPSEEK_KEY" ] && { warn "DeepSeek API Key 未填写 — AI 对话和内容审核将不可用"; MISSING=$((MISSING+1)); }
[ -z "$BAIDU_APPID" ]  && { warn "百度翻译 APPID 未填写 — 翻译功能将不可用"; MISSING=$((MISSING+1)); }
[ -z "$BAIDU_SECRET" ] && { warn "百度翻译密钥未填写 — 翻译功能将不可用"; MISSING=$((MISSING+1)); }

if [ $MISSING -gt 0 ]; then
    echo ""
    warn "有 $MISSING 项密钥未填写，稍后请编辑补充:"
    echo -e "  ${YELLOW}nano $ENV_FILE${NC}"
fi

echo ""
ok "阶段 4 完成 ✓"
echo -e "  下一步: ${BLUE}sudo bash $INSTALL_DIR/scripts/deploy/stage5-build.sh${NC}"
