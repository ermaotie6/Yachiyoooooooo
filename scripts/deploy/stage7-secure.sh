#!/usr/bin/env bash
# ============================================================
# Yachiyo 部署 — 阶段 7: HTTPS 与安全加固
# SSL 证书部署 + Nginx SSL 启用 + 安全检查
# 用法: sudo bash scripts/deploy/stage7-secure.sh
# ============================================================

set -euo pipefail

GREEN="\033[0;32m"; YELLOW="\033[1;33m"; RED="\033[0;31m"; BLUE="\033[0;34m"; CYAN="\033[0;36m"; NC="\033[0m"
info()  { echo -e "${BLUE}[阶段7]${NC} $*"; }
ok()    { echo -e "${GREEN}[✓]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
fail()  { echo -e "${RED}[✗]${NC} $*"; }

INSTALL_DIR="${INSTALL_DIR:-/opt/yachiyo}"
SSL_DIR="$INSTALL_DIR/ssl"
NGINX_CONF="$INSTALL_DIR/nginx.conf"

echo -e "${CYAN}╔══════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  阶段 7: HTTPS 与安全加固             ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════╝${NC}"
echo ""

cd "$INSTALL_DIR"

# 检测 Docker Compose
if docker compose version &>/dev/null; then
    DC="docker compose"
elif command -v docker-compose &>/dev/null; then
    DC="docker-compose"
else
    DC=""
fi

# ═════════════════════════════════════
# 步骤 1: SSL 证书部署
# ═════════════════════════════════════
echo -e "${BLUE}[步骤 1] SSL 证书部署${NC}"
echo ""

mkdir -p "$SSL_DIR"

SKIP_SSL=false

if [ -f "$SSL_DIR/cert.pem" ] && [ -f "$SSL_DIR/key.pem" ]; then
    ok "SSL 证书已存在: $SSL_DIR/"
else
    warn "未检测到 SSL 证书文件"
    echo ""
    echo "请选择证书部署方式:"
    echo "  1) 手动粘贴证书内容 (Cloudflare Origin Certificate)"
    echo "  2) 从本地路径复制证书文件"
    echo "  3) 跳过 HTTPS 配置"
    read -p "选择 (1/2/3): " -n 1 -r
    echo ""

    case "$REPLY" in
        1)
            echo ""
            echo "请粘贴 Origin Certificate 内容 (以 -----BEGIN CERTIFICATE----- 开头):"
            echo "粘贴完成后，单独一行输入 EOF 并回车:"
            CERT_CONTENT=""
            while IFS= read -r line; do
                [ "$line" = "EOF" ] && break
                CERT_CONTENT+="$line"$'\n'
            done
            echo "$CERT_CONTENT" > "$SSL_DIR/cert.pem"

            echo ""
            echo "请粘贴 Private Key 内容 (以 -----BEGIN PRIVATE KEY----- 开头):"
            echo "粘贴完成后，单独一行输入 EOF 并回车:"
            KEY_CONTENT=""
            while IFS= read -r line; do
                [ "$line" = "EOF" ] && break
                KEY_CONTENT+="$line"$'\n'
            done
            echo "$KEY_CONTENT" > "$SSL_DIR/key.pem"
            ;;
        2)
            read -p "cert.pem 路径: " CERT_PATH
            read -p "key.pem 路径: " KEY_PATH
            if [ -f "$CERT_PATH" ] && [ -f "$KEY_PATH" ]; then
                cp "$CERT_PATH" "$SSL_DIR/cert.pem"
                cp "$KEY_PATH" "$SSL_DIR/key.pem"
            else
                fail "证书文件不存在"
                SKIP_SSL=true
            fi
            ;;
        3|*)
            SKIP_SSL=true
            warn "跳过 HTTPS 配置"
            ;;
    esac

    if [ -f "$SSL_DIR/cert.pem" ] && [ -f "$SSL_DIR/key.pem" ]; then
        chmod 644 "$SSL_DIR/cert.pem"
        chmod 600 "$SSL_DIR/key.pem"
        ok "SSL 证书文件已就位"
    fi
fi

# ═════════════════════════════════════
# 步骤 2: Nginx SSL 配置
# ═════════════════════════════════════
if [ "$SKIP_SSL" = false ] && [ -f "$SSL_DIR/cert.pem" ] && [ -f "$SSL_DIR/key.pem" ]; then
    echo ""
    echo -e "${BLUE}[步骤 2] Nginx SSL 配置${NC}"
    echo ""
    info "SSL 证书路径: $SSL_DIR/"
    info "docker-compose.yml 已将 ./ssl 挂载到容器内 /etc/nginx/ssl"
    echo ""
    info "请编辑 nginx.conf 取消 SSL 相关注释:"
    echo "  取消注释以下行:"
    echo "    listen 443 ssl http2;"
    echo "    ssl_certificate /etc/nginx/ssl/cert.pem;"
    echo "    ssl_certificate_key /etc/nginx/ssl/key.pem;"
    echo "    ssl_protocols TLSv1.2 TLSv1.3;"
    echo ""
    read -p "是否现在编辑 nginx.conf? (y/N) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        ${EDITOR:-nano} "$NGINX_CONF"
    fi

    if [ -n "$DC" ]; then
        info "重启 Nginx..."
        $DC restart nginx
        sleep 2
        ok "Nginx 已重启"
    fi
fi

# ═════════════════════════════════════
# 步骤 3: 安全加固检查
# ═════════════════════════════════════
echo ""
echo -e "${BLUE}[步骤 3] 安全加固检查表${NC}"
echo ""

SEC_PASS=0
SEC_WARN=0

# 检查 JWT 密钥长度
JWT_KEY=$(grep "JWT_SECRET_KEY=" "$INSTALL_DIR/.env" 2>/dev/null | cut -d= -f2)
JWT_LEN=${#JWT_KEY}
if [ "$JWT_LEN" -ge 32 ]; then
    ok "JWT 密钥长度: $JWT_LEN 字符 (≥32 ✓)"
    SEC_PASS=$((SEC_PASS+1))
else
    warn "JWT 密钥长度: $JWT_LEN 字符 (建议 ≥32)"
    SEC_WARN=$((SEC_WARN+1))
fi

# 检查数据库密码
DB_PASS=$(grep "DATABASE_PASSWORD=" "$INSTALL_DIR/.env" 2>/dev/null | cut -d= -f2)
if [ "$DB_PASS" = "postgres" ] || [ -z "$DB_PASS" ]; then
    warn "数据库密码仍为默认值 'postgres'，生产环境请修改"
    SEC_WARN=$((SEC_WARN+1))
else
    ok "数据库密码已自定义"
    SEC_PASS=$((SEC_PASS+1))
fi

# 检查 CORS
if grep -q 'cors_origins: "\*"' "$INSTALL_DIR/backend/config/config.yaml" 2>/dev/null; then
    warn "CORS 设为 * (允许所有域名)，生产环境请改为你的域名"
    SEC_WARN=$((SEC_WARN+1))
else
    ok "CORS 配置正常"
    SEC_PASS=$((SEC_PASS+1))
fi

# 检查管理工具
if [ -n "$DC" ]; then
    MGMT_RUNNING=false
    $DC ps pgadmin 2>/dev/null | grep -qi "up" && MGMT_RUNNING=true
    $DC ps redis-commander 2>/dev/null | grep -qi "up" && MGMT_RUNNING=true

    if [ "$MGMT_RUNNING" = true ]; then
        warn "pgAdmin 或 Redis Commander 正在运行 — 生产环境建议关闭:"
        echo "  $DC stop pgadmin redis-commander"
        SEC_WARN=$((SEC_WARN+1))
    else
        ok "管理工具未对外暴露"
        SEC_PASS=$((SEC_PASS+1))
    fi
fi

# 检查 .env 权限
ENV_PERMS=$(stat -c %a "$INSTALL_DIR/.env" 2>/dev/null || echo "unknown")
if [ "$ENV_PERMS" = "600" ]; then
    ok ".env 文件权限: 600 (仅 owner 可读写)"
    SEC_PASS=$((SEC_PASS+1))
else
    warn ".env 文件权限: $ENV_PERMS (建议设为 600)"
    echo "  chmod 600 $INSTALL_DIR/.env"
    SEC_WARN=$((SEC_WARN+1))
fi

echo ""
echo -e "${CYAN}════════════════════════════════════════${NC}"
echo -e "  安全通过: ${GREEN}$SEC_PASS${NC}  |  安全警告: ${YELLOW}$SEC_WARN${NC}"
echo -e "${CYAN}════════════════════════════════════════${NC}"

echo ""
ok "阶段 7 完成 ✓"
echo ""
echo -e "${GREEN}🎀 Yachiyo 全部部署流程完成！${NC}"
