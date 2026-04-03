#!/usr/bin/env bash
# ============================================================
# Yachiyo 一键部署脚本
# 支持: Arch Linux / Debian / Ubuntu
# 用途: 在全新 Linux 服务器上自动安装依赖并启动全部服务
# ============================================================

set -euo pipefail

# ==================== 颜色输出 ====================
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

info()    { echo -e "${BLUE}[INFO]${NC} $*"; }
success() { echo -e "${GREEN}[OK]${NC} $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; }
step()    { echo -e "\n${CYAN}========== $* ==========${NC}"; }

# ==================== 配置变量 ====================
PROJECT_NAME="yachiyo"
REPO_URL="https://github.com/ermaotie6/yachiyoooooooo.git"
INSTALL_DIR="${INSTALL_DIR:-/opt/yachiyo}"
BRANCH="${BRANCH:-main}"
DEPLOY_MODE="${1:-docker}"  # docker (默认) 或 native

# Docker Compose 模式下只启动核心服务 (不含监控/管理工具)
CORE_SERVICES="postgres redis backend frontend nginx"
ALL_SERVICES="postgres redis backend frontend nginx prometheus grafana"

# ==================== 检测发行版 ====================
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO_ID="${ID}"
        DISTRO_NAME="${PRETTY_NAME}"
    elif [ -f /etc/arch-release ]; then
        DISTRO_ID="arch"
        DISTRO_NAME="Arch Linux"
    else
        error "无法识别的 Linux 发行版"
        exit 1
    fi

    case "$DISTRO_ID" in
        arch|manjaro|endeavouros)
            DISTRO_FAMILY="arch"
            PKG_INSTALL="sudo pacman -S --noconfirm --needed"
            PKG_UPDATE="sudo pacman -Syu --noconfirm"
            ;;
        debian|ubuntu|linuxmint|pop)
            DISTRO_FAMILY="debian"
            PKG_INSTALL="sudo apt-get install -y"
            PKG_UPDATE="sudo apt-get update && sudo apt-get upgrade -y"
            ;;
        *)
            error "不支持的发行版: $DISTRO_ID"
            error "本脚本仅支持 Arch Linux 和 Debian/Ubuntu 系列"
            exit 1
            ;;
    esac

    success "检测到系统: $DISTRO_NAME (${DISTRO_FAMILY} 系列)"
}

# ==================== 安装基础依赖 ====================
install_base_deps() {
    step "安装基础依赖"

    info "更新系统包..."
    eval "$PKG_UPDATE"

    case "$DISTRO_FAMILY" in
        arch)
            $PKG_INSTALL base-devel git curl wget openssl ca-certificates
            ;;
        debian)
            $PKG_INSTALL build-essential git curl wget openssl ca-certificates \
                apt-transport-https gnupg lsb-release software-properties-common
            ;;
    esac

    success "基础依赖安装完成"
}

# ==================== 安装 Docker ====================
install_docker() {
    step "安装 Docker"

    if command -v docker &>/dev/null; then
        success "Docker 已安装: $(docker --version)"
    else
        info "正在安装 Docker..."

        case "$DISTRO_FAMILY" in
            arch)
                $PKG_INSTALL docker docker-compose docker-buildx
                ;;
            debian)
                # 使用官方 Docker 仓库
                sudo install -m 0755 -d /etc/apt/keyrings
                curl -fsSL "https://download.docker.com/linux/${DISTRO_ID}/gpg" | \
                    sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg
                sudo chmod a+r /etc/apt/keyrings/docker.gpg

                echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] \
                    https://download.docker.com/linux/${DISTRO_ID} \
                    $(lsb_release -cs) stable" | \
                    sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

                sudo apt-get update
                $PKG_INSTALL docker-ce docker-ce-cli containerd.io \
                    docker-buildx-plugin docker-compose-plugin
                ;;
        esac

        success "Docker 安装完成"
    fi

    # 确保 Docker 服务运行
    sudo systemctl enable --now docker

    # 将当前用户加入 docker 组 (避免每次 sudo)
    if ! groups "$USER" | grep -q docker; then
        sudo usermod -aG docker "$USER"
        warn "已将 $USER 加入 docker 组，部分命令可能需要重新登录后生效"
        warn "如果后续遇到权限问题，请运行: newgrp docker"
    fi

    # 检查 docker compose 子命令 (v2) 或独立命令
    if docker compose version &>/dev/null; then
        COMPOSE_CMD="docker compose"
    elif command -v docker-compose &>/dev/null; then
        COMPOSE_CMD="docker-compose"
    else
        error "Docker Compose 未安装"
        exit 1
    fi

    success "Docker Compose 可用: $COMPOSE_CMD"
}

# ==================== 安装原生编译依赖 (仅 native 模式) ====================
install_native_deps() {
    step "安装原生编译依赖 (C++/Node.js)"

    case "$DISTRO_FAMILY" in
        arch)
            $PKG_INSTALL cmake gcc ninja postgresql redis \
                openssl libpqxx hiredis nlohmann-json yaml-cpp \
                nodejs npm python python-pip
            ;;
        debian)
            # C++ 编译工具链
            $PKG_INSTALL cmake g++ ninja-build \
                libssl-dev libpq-dev libhiredis-dev libcurl4-openssl-dev \
                nlohmann-json3-dev libyaml-cpp-dev pkg-config

            # Node.js (通过 NodeSource 安装 v18+)
            if ! command -v node &>/dev/null || [ "$(node -v | cut -d. -f1 | tr -d v)" -lt 18 ]; then
                info "安装 Node.js 18..."
                curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
                $PKG_INSTALL nodejs
            fi

            # PostgreSQL 和 Redis
            $PKG_INSTALL postgresql postgresql-contrib redis-server

            # Python
            $PKG_INSTALL python3 python3-pip python3-venv
            ;;
    esac

    success "原生编译依赖安装完成"
}

# ==================== 克隆/更新项目代码 ====================
setup_project() {
    step "部署项目代码"

    if [ -d "$INSTALL_DIR/.git" ]; then
        info "项目目录已存在，拉取最新代码..."
        cd "$INSTALL_DIR"
        git fetch origin
        git reset --hard "origin/$BRANCH"
        git clean -fd
    else
        info "克隆项目到 $INSTALL_DIR ..."
        sudo mkdir -p "$INSTALL_DIR"
        sudo chown "$USER:$USER" "$INSTALL_DIR"
        git clone --branch "$BRANCH" "$REPO_URL" "$INSTALL_DIR"
        cd "$INSTALL_DIR"
    fi

    success "项目代码就绪: $(git log --oneline -1)"
}

# ==================== 配置环境变量 ====================
setup_env() {
    step "配置环境变量"

    ENV_FILE="$INSTALL_DIR/.env"

    if [ -f "$ENV_FILE" ]; then
        warn ".env 文件已存在，跳过创建 (如需重新配置请手动编辑)"
    else
        info "创建 .env 文件..."

        # 生成随机 JWT 密钥
        JWT_SECRET=$(openssl rand -hex 32)

        cat > "$ENV_FILE" <<ENVEOF
# Yachiyo 环境变量 — 由部署脚本自动生成于 $(date '+%Y-%m-%d %H:%M:%S')
# !! 请修改以下值为你的实际配置 !!

# ===== 数据库 =====
DATABASE_PASSWORD=postgres

# ===== JWT =====
JWT_SECRET_KEY=${JWT_SECRET}

# ===== DeepSeek API (AI 对话 + 内容审核) =====
DEEPSEEK_API_KEY=

# ===== 百度翻译 =====
BAIDU_TRANSLATE_APP_ID=
BAIDU_TRANSLATE_API_KEY=

# ===== GPT-SoVITS 语音合成 (可选) =====
SOVITS_API_KEY=

# ===== 部署模式 =====
ENVIRONMENT=production
ENVEOF

        chmod 600 "$ENV_FILE"
        success ".env 文件已创建 (JWT 密钥已自动生成)"
        warn "⚠️  请编辑 $ENV_FILE 填入 API 密钥:"
        warn "   nano $ENV_FILE"
    fi
}

# ==================== Docker 模式部署 ====================
deploy_docker() {
    step "Docker 模式部署"

    cd "$INSTALL_DIR"

    # 加载 .env
    set -a
    [ -f .env ] && source .env
    set +a

    info "构建并启动核心服务..."
    $COMPOSE_CMD build --parallel
    $COMPOSE_CMD up -d $CORE_SERVICES

    info "等待服务启动..."
    sleep 10

    # 健康检查
    check_health_docker

    echo ""
    success "🎉 Yachiyo 部署完成！"
    echo ""
    print_access_info
    echo ""

    info "启动监控服务 (Prometheus + Grafana):"
    echo "  $COMPOSE_CMD up -d prometheus grafana"
    echo ""
    info "查看日志:"
    echo "  $COMPOSE_CMD logs -f backend"
    echo ""
    info "停止所有服务:"
    echo "  $COMPOSE_CMD down"
}

# ==================== 原生模式部署 ====================
deploy_native() {
    step "原生模式部署"

    cd "$INSTALL_DIR"

    # --- 编译后端 ---
    info "编译 C++ 后端..."
    cd backend
    mkdir -p build && cd build
    cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
    ninja -j"$(nproc)"
    cd "$INSTALL_DIR"
    success "后端编译完成"

    # --- 构建前端 ---
    info "构建 Vue 前端..."
    cd frontend
    npm ci --prefer-offline --no-audit 2>/dev/null || npm install
    npm run build
    cd "$INSTALL_DIR"
    success "前端构建完成"

    # --- 启动数据库 (如果用 Docker) ---
    if command -v docker &>/dev/null; then
        info "使用 Docker 启动 PostgreSQL 和 Redis..."
        $COMPOSE_CMD up -d postgres redis
        sleep 5
    else
        info "请确保 PostgreSQL 和 Redis 已手动启动"
        # 启动系统服务
        sudo systemctl enable --now postgresql redis 2>/dev/null || true
    fi

    # --- 创建 systemd 服务 ---
    setup_systemd_service

    # --- 配置 Nginx ---
    setup_nginx_native

    success "🎉 Yachiyo 原生部署完成！"
    echo ""
    print_access_info
}

# ==================== 创建 systemd 服务 ====================
setup_systemd_service() {
    info "创建 systemd 服务..."

    # 创建专用用户
    if ! id yachiyo &>/dev/null; then
        sudo useradd -r -s /usr/sbin/nologin -d "$INSTALL_DIR" yachiyo
    fi
    sudo chown -R yachiyo:yachiyo "$INSTALL_DIR/backend/build" "$INSTALL_DIR/backend/config"
    sudo mkdir -p "$INSTALL_DIR/logs"
    sudo chown yachiyo:yachiyo "$INSTALL_DIR/logs"

    sudo tee /etc/systemd/system/yachiyo-backend.service > /dev/null <<SVCEOF
[Unit]
Description=Yachiyo Backend Server
After=network.target postgresql.service redis.service
Wants=postgresql.service redis.service

[Service]
Type=simple
User=yachiyo
Group=yachiyo
WorkingDirectory=$INSTALL_DIR/backend
ExecStart=$INSTALL_DIR/backend/build/src/yachiyo_cpp --config-dir $INSTALL_DIR/backend/config --env production
Restart=always
RestartSec=5
EnvironmentFile=$INSTALL_DIR/.env

# 安全加固
NoNewPrivileges=true
ProtectSystem=strict
ReadWritePaths=$INSTALL_DIR/logs /tmp
PrivateTmp=true

[Install]
WantedBy=multi-user.target
SVCEOF

    sudo systemctl daemon-reload
    sudo systemctl enable --now yachiyo-backend
    success "systemd 服务已创建并启动"
}

# ==================== 配置 Nginx (原生模式) ====================
setup_nginx_native() {
    info "配置 Nginx..."

    case "$DISTRO_FAMILY" in
        arch)   $PKG_INSTALL nginx ;;
        debian) $PKG_INSTALL nginx ;;
    esac

    # 部署前端静态文件
    sudo mkdir -p /var/www/yachiyo
    sudo cp -r "$INSTALL_DIR/frontend/dist/"* /var/www/yachiyo/
    sudo chown -R www-data:www-data /var/www/yachiyo 2>/dev/null || \
        sudo chown -R http:http /var/www/yachiyo 2>/dev/null || true

    # 生成 Nginx 站点配置
    local NGINX_CONF
    case "$DISTRO_FAMILY" in
        arch)   NGINX_CONF="/etc/nginx/sites-available" ;;
        debian) NGINX_CONF="/etc/nginx/sites-available" ;;
    esac

    sudo mkdir -p "$NGINX_CONF" /etc/nginx/sites-enabled

    sudo tee "$NGINX_CONF/yachiyo" > /dev/null <<'NGXEOF'
server {
    listen 80;
    server_name _;

    # 前端静态文件
    root /var/www/yachiyo;
    index index.html;

    # SPA 路由回退
    location / {
        try_files $uri $uri/ /index.html;
    }

    # 静态资源长缓存
    location /assets/ {
        expires 30d;
        add_header Cache-Control "public, immutable";
    }

    # API 代理
    location /api/ {
        proxy_pass http://127.0.0.1:8080;
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        proxy_connect_timeout 10s;
        proxy_read_timeout 30s;
    }

    # WebSocket 代理
    location /ws {
        proxy_pass http://127.0.0.1:9001;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_read_timeout 86400s;
        proxy_send_timeout 86400s;
    }

    # 健康检查
    location /health {
        proxy_pass http://127.0.0.1:8080/api/v1/health;
    }
}
NGXEOF

    sudo ln -sf "$NGINX_CONF/yachiyo" /etc/nginx/sites-enabled/yachiyo
    sudo rm -f /etc/nginx/sites-enabled/default 2>/dev/null || true

    # 确保 nginx.conf 中 include sites-enabled (Arch 默认不 include)
    if [ "$DISTRO_FAMILY" = "arch" ]; then
        if ! grep -q "sites-enabled" /etc/nginx/nginx.conf; then
            sudo sed -i '/http {/a \    include /etc/nginx/sites-enabled/*;' /etc/nginx/nginx.conf
        fi
    fi

    sudo nginx -t && sudo systemctl enable --now nginx && sudo systemctl reload nginx
    success "Nginx 配置完成"
}

# ==================== 健康检查 ====================
check_health_docker() {
    info "健康检查..."
    local MAX_RETRIES=12
    local RETRY=0

    while [ $RETRY -lt $MAX_RETRIES ]; do
        if curl -sf http://localhost:8080/api/v1/health > /dev/null 2>&1; then
            success "后端服务正常运行"
            return 0
        fi
        RETRY=$((RETRY + 1))
        info "等待后端启动... ($RETRY/$MAX_RETRIES)"
        sleep 5
    done

    warn "后端健康检查超时，请手动检查:"
    echo "  $COMPOSE_CMD logs backend"
    return 1
}

# ==================== 打印访问信息 ====================
print_access_info() {
    echo -e "${GREEN}╔══════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║       🎀 Yachiyo 部署成功！                  ║${NC}"
    echo -e "${GREEN}╠══════════════════════════════════════════════╣${NC}"
    echo -e "${GREEN}║  前端:      http://localhost:3000            ║${NC}"
    echo -e "${GREEN}║  后端 API:  http://localhost:8080/api/v1     ║${NC}"
    echo -e "${GREEN}║  WebSocket: ws://localhost:9001              ║${NC}"
    echo -e "${GREEN}║  Nginx:     http://localhost                 ║${NC}"
    echo -e "${GREEN}╠══════════════════════════════════════════════╣${NC}"
    echo -e "${GREEN}║  pgAdmin:   http://localhost:5050            ║${NC}"
    echo -e "${GREEN}║  Grafana:   http://localhost:3001            ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${YELLOW}⚠️  首次部署请检查:${NC}"
    echo "  1. 编辑 $INSTALL_DIR/.env 填入 API 密钥"
    echo "  2. 重启服务使配置生效"
}

# ==================== 卸载 ====================
uninstall() {
    step "卸载 Yachiyo"

    cd "$INSTALL_DIR" 2>/dev/null || true

    if [ -n "${COMPOSE_CMD:-}" ]; then
        $COMPOSE_CMD down -v 2>/dev/null || true
    fi

    sudo systemctl stop yachiyo-backend 2>/dev/null || true
    sudo systemctl disable yachiyo-backend 2>/dev/null || true
    sudo rm -f /etc/systemd/system/yachiyo-backend.service
    sudo systemctl daemon-reload

    sudo rm -f /etc/nginx/sites-enabled/yachiyo
    sudo rm -f /etc/nginx/sites-available/yachiyo
    sudo rm -rf /var/www/yachiyo
    sudo systemctl reload nginx 2>/dev/null || true

    warn "项目目录 $INSTALL_DIR 未删除，如需删除请手动执行:"
    echo "  sudo rm -rf $INSTALL_DIR"

    success "卸载完成"
}

# ==================== 帮助 ====================
show_help() {
    echo "用法: $0 [模式] [选项]"
    echo ""
    echo "部署模式:"
    echo "  docker    使用 Docker Compose 部署全部服务 (默认)"
    echo "  native    原生编译部署 (需要安装编译工具链)"
    echo "  uninstall 卸载服务"
    echo "  help      显示帮助"
    echo ""
    echo "环境变量:"
    echo "  INSTALL_DIR   安装目录 (默认: /opt/yachiyo)"
    echo "  BRANCH        Git 分支 (默认: main)"
    echo ""
    echo "示例:"
    echo "  # Docker 部署 (推荐)"
    echo "  curl -fsSL https://raw.githubusercontent.com/ermaotie6/yachiyoooooooo/main/scripts/deploy-server.sh | bash"
    echo ""
    echo "  # 原生编译部署"
    echo "  ./scripts/deploy-server.sh native"
    echo ""
    echo "  # 自定义安装目录"
    echo "  INSTALL_DIR=/home/user/yachiyo ./scripts/deploy-server.sh docker"
}

# ==================== 主流程 ====================
main() {
    echo -e "${CYAN}"
    echo "  ╔═══════════════════════════════════════╗"
    echo "  ║  🎀 Yachiyo 一键部署脚本 v1.0        ║"
    echo "  ║  AI 虚拟形象直播平台                  ║"
    echo "  ╚═══════════════════════════════════════╝"
    echo -e "${NC}"

    case "${1:-docker}" in
        help|-h|--help)
            show_help
            exit 0
            ;;
        uninstall)
            detect_distro
            install_docker 2>/dev/null || true
            uninstall
            exit 0
            ;;
        docker)
            detect_distro
            install_base_deps
            install_docker
            setup_project
            setup_env
            deploy_docker
            ;;
        native)
            detect_distro
            install_base_deps
            install_docker  # 仍装 Docker 用于数据库
            install_native_deps
            setup_project
            setup_env
            deploy_native
            ;;
        *)
            error "未知模式: $1"
            show_help
            exit 1
            ;;
    esac
}

main "$@"
