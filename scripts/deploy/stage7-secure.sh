#!/usr/bin/env bash
# ============================================================
# Yachiyo 部署 — 阶段 7: HTTPS 与安全加固
# SSL 证书部署 + Nginx SSL 自动配置 + 安全检查
# 用法: sudo bash scripts/deploy/stage7-secure.sh
#
# 优化说明:
#   - SSL 证书和域名在 deploy 脚本执行时填写（不需要提前手改 nginx.conf）
#   - 自动用 sed 取消 nginx.conf 中 SSL 相关注释
#   - 自动设置 server_name 为你的域名
#   - 自动启用 HTTP→HTTPS 重定向
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
# 步骤 0: 询问域名
# ═════════════════════════════════════
echo -e "${BLUE}[步骤 0] 域名配置${NC}"
echo ""

if grep -q "yachiyo\.com" "$NGINX_CONF" 2>/dev/null && \
   ! grep -q "^[[:space:]]*server_name[[:space:]]*[^l]" "$NGINX_CONF" 2>/dev/null; then
    # nginx.conf 中 server_name 还是 localhost → 询问
    # 优先从 .env 读取
    DOMAIN_FROM_ENV=$(grep "^YACHIYO_DOMAIN=" "$INSTALL_DIR/.env" 2>/dev/null | cut -d= -f2)
    if [ -n "$DOMAIN_FROM_ENV" ]; then
        ok "从 .env 读取域名: ${GREEN}$DOMAIN_FROM_ENV${NC}"
        DOMAIN="$DOMAIN_FROM_ENV"
    else
        read -p "  域名 (如 yachiyo.example.com，留空跳过 HTTPS): " DOMAIN
    fi
else
    # 获取当前 server_name
    CURRENT_DOMAIN=$(grep "server_name" "$NGINX_CONF" 2>/dev/null | head -1 | awk '{print $2}' | tr -d ';')
    if [ "$CURRENT_DOMAIN" != "localhost" ] && [ -n "$CURRENT_DOMAIN" ]; then
        ok "域名已配置: ${GREEN}$CURRENT_DOMAIN${NC}"
        DOMAIN="$CURRENT_DOMAIN"
    else
        DOMAIN_FROM_ENV=$(grep "^YACHIYO_DOMAIN=" "$INSTALL_DIR/.env" 2>/dev/null | cut -d= -f2)
        if [ -n "$DOMAIN_FROM_ENV" ]; then
            ok "从 .env 读取域名: ${GREEN}$DOMAIN_FROM_ENV${NC}"
            DOMAIN="$DOMAIN_FROM_ENV"
        else
            read -p "  域名 (如 yachiyo.example.com，留空跳过 HTTPS): " DOMAIN
        fi
    fi
fi
echo ""

# ═════════════════════════════════════
# 步骤 1: SSL 证书部署
# ═════════════════════════════════════
echo -e "${BLUE}[步骤 1] SSL 证书部署${NC}"
echo ""

mkdir -p "$SSL_DIR"

SKIP_SSL=false

if [ -z "$DOMAIN" ]; then
    SKIP_SSL=true
    warn "未填写域名，跳过 HTTPS 配置"
fi

if [ "$SKIP_SSL" = false ] && [ -f "$SSL_DIR/cert.pem" ] && [ -f "$SSL_DIR/key.pem" ]; then
    ok "SSL 证书已存在: $SSL_DIR/"
    echo ""
    read -p "  重新部署证书? (y/N) " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        info "保留现有证书"
    else
        SKIP_SSL=false  # 需要重新部署
    fi
fi

if [ "$SKIP_SSL" = false ] && [ -n "$DOMAIN" ]; then
    echo "请选择证书部署方式:"
    echo "  1) 手动粘贴证书内容 (Cloudflare Origin Certificate)"
    echo "  2) 从本地路径复制证书文件"
    echo "  3) 使用 acme.sh / certbot 自动申请 (Let's Encrypt)"
    echo "  4) 跳过 HTTPS 配置"
    read -p "选择 (1/2/3/4): " -n 1 -r
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
        3)
            echo ""
            info "使用 Let's Encrypt 自动申请证书 (需要域名 DNS 已指向本机)..."
            
            # 检查 certbot 是否安装
            if ! command -v certbot &>/dev/null; then
                info "安装 certbot..."
                if command -v pacman &>/dev/null; then
                    pacman -S --noconfirm certbot certbot-nginx 2>/dev/null || true
                elif command -v apt &>/dev/null; then
                    apt install -y certbot python3-certbot-nginx 2>/dev/null || true
                fi
            fi

            if command -v certbot &>/dev/null; then
                # 先确保 80 端口可达
                info "申请证书前请确认:"
                echo "  1. 域名 $DOMAIN 的 DNS 已指向本机 IP"
                echo "  2. 防火墙已开放 80/443 端口"
                read -p "  确认继续? (y/N) " -n 1 -r
                echo ""
                if [[ $REPLY =~ ^[Yy]$ ]]; then
                    # certbot standalone 模式（不需要 nginx 先运行）
                    if certbot certonly --standalone -d "$DOMAIN" \
                        --non-interactive --agree-tos --email "admin@${DOMAIN}" \
                        --cert-name yachiyo 2>/dev/null; then
                        cp /etc/letsencrypt/live/yachiyo/fullchain.pem "$SSL_DIR/cert.pem"
                        cp /etc/letsencrypt/live/yachiyo/privkey.pem "$SSL_DIR/key.pem"
                        ok "Let's Encrypt 证书已获取"
                    else
                        warn "certbot 申请失败，请手动部署证书"
                        SKIP_SSL=true
                    fi
                else
                    SKIP_SSL=true
                fi
            else
                warn "certbot 未安装，回退到手动模式"
                SKIP_SSL=true
            fi
            ;;
        4|*)
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
# 步骤 2: 自动配置 Nginx SSL
# ═════════════════════════════════════
if [ "$SKIP_SSL" = false ] && [ -n "$DOMAIN" ] && [ -f "$SSL_DIR/cert.pem" ] && [ -f "$SSL_DIR/key.pem" ]; then
    echo ""
    echo -e "${BLUE}[步骤 2] 自动配置 Nginx SSL${NC}"
    echo ""

    # 备份原配置
    BACKUP_NGINX="${NGINX_CONF}.backup.$(date +%Y%m%d_%H%M%S)"
    cp "$NGINX_CONF" "$BACKUP_NGINX"
    ok "已备份 nginx.conf → ${BACKUP_NGINX##*/}"

    # --- 使用 sed 自动修改 nginx.conf ---

    # 2a. 替换 server_name localhost → 域名
    sed -i "s/server_name localhost;/server_name ${DOMAIN};/" "$NGINX_CONF"

    # 2b. 取消 SSL listen 注释 (listen 443 ssl http2;)
    sed -i 's/^[[:space:]]*#[[:space:]]*listen 443 ssl http2;/        listen 443 ssl http2;/' "$NGINX_CONF"

    # 2c. 取消 SSL 证书路径注释
    sed -i 's/^[[:space:]]*#[[:space:]]*ssl_certificate \/etc\/nginx\/ssl\/cert\.pem;/        ssl_certificate \/etc\/nginx\/ssl\/cert.pem;/' "$NGINX_CONF"
    sed -i 's/^[[:space:]]*#[[:space:]]*ssl_certificate_key \/etc\/nginx\/ssl\/key\.pem;/        ssl_certificate_key \/etc\/nginx\/ssl\/key.pem;/' "$NGINX_CONF"

    # 2d. 取消 SSL 协议/加密套件注释
    sed -i 's/^[[:space:]]*#[[:space:]]*ssl_protocols TLSv1\.2 TLSv1\.3;/        ssl_protocols TLSv1.2 TLSv1.3;/' "$NGINX_CONF"
    sed -i 's/^[[:space:]]*#[[:space:]]*ssl_ciphers HIGH:!aNULL:!MD5;/        ssl_ciphers HIGH:!aNULL:!MD5;/' "$NGINX_CONF"

    # 2e. 启用 HTTP→HTTPS 重定向（取消整个 server 块注释）
    #     nginx.conf 中 HTTP redirect server 块是：
    #     # server { listen 80; server_name yachiyo.com; return 301 https://... }
    sed -i "/^[[:space:]]*# HTTP → HTTPS 重定向/,/^[[:space:]]*#[[:space:]]*}/{
        s/^[[:space:]]*#[[:space:]]*//
    }" "$NGINX_CONF"

    # 2f. 把 redirect 块的 server_name 也替换为域名
    sed -i "s/server_name yachiyo\.com;/server_name ${DOMAIN};/" "$NGINX_CONF"

    ok "Nginx SSL 配置已自动完成:"

    echo ""
    echo -e "  ${CYAN}已自动修改:${NC}"
    echo "    • server_name: localhost → ${GREEN}${DOMAIN}${NC}"
    echo "    • SSL listen 443: 已启用"
    echo "    • SSL 证书路径: 已配置"
    echo "    • HTTP→HTTPS 重定向: 已启用"

    # 重启 Nginx 使配置生效
    if [ -n "$DC" ]; then
        echo ""
        info "重启 Nginx 使 SSL 配置生效..."
        $DC restart nginx
        sleep 2
        
        # 验证
        if $DC ps nginx 2>/dev/null | grep -qi "up"; then
            ok "Nginx 已重启，HTTPS 已生效"
            echo ""
            echo -e "  访问: ${GREEN}https://${DOMAIN}${NC}"
        else
            warn "Nginx 重启可能失败，检查配置:"
            echo "  $DC logs nginx --tail 20"
        fi
    fi
else
    echo ""
    info "跳过 Nginx SSL 配置 (证书未就绪或已跳过)"
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
DB_PASS=$(grep "^DB_PASSWORD=" "$INSTALL_DIR/.env" 2>/dev/null | cut -d= -f2)
if [ "$DB_PASS" = "postgres" ] || [ "$DB_PASS" = "postgres_dev_only" ] || [ -z "$DB_PASS" ]; then
    warn "数据库密码仍为默认值，生产环境请修改"
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

# 检查 OPENCLAW_GATEWAY_TOKEN
OC_TOKEN=$(grep "^OPENCLAW_GATEWAY_TOKEN=" "$INSTALL_DIR/.env" 2>/dev/null | cut -d= -f2)
if [ -z "$OC_TOKEN" ]; then
    warn "OPENCLAW_GATEWAY_TOKEN 未配置 — AI 功能将不可用"
    SEC_WARN=$((SEC_WARN+1))
else
    ok "OpenClaw 令牌已配置"
    SEC_PASS=$((SEC_PASS+1))
fi

echo ""
echo -e "${CYAN}════════════════════════════════════════${NC}"
echo -e "  安全通过: ${GREEN}$SEC_PASS${NC}  |  安全警告: ${YELLOW}$SEC_WARN${NC}"
echo -e "${CYAN}════════════════════════════════════════${NC}"

echo ""
ok "阶段 7 完成 ✓"
echo ""
if [ "$SKIP_SSL" = false ] && [ -n "$DOMAIN" ]; then
    echo -e "${GREEN}🎀 Yachiyo 全部部署流程完成！${NC}"
    echo -e "  HTTPS: ${GREEN}https://${DOMAIN}${NC}"
else
    echo -e "${GREEN}🎀 Yachiyo 部署完成（HTTP 模式）${NC}"
    echo -e "  后续可手动执行 HTTPS 加固: sudo bash $0"
fi
