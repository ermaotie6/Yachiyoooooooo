#!/usr/bin/env bash
# ============================================================
# Yachiyo 部署 — 阶段 2: 环境安装
# 安装 Docker 和 Docker Compose
# 用法: sudo bash scripts/deploy/stage2-install.sh
# ============================================================

set -euo pipefail

GREEN="\033[0;32m"; YELLOW="\033[1;33m"; RED="\033[0;31m"; BLUE="\033[0;34m"; CYAN="\033[0;36m"; NC="\033[0m"
info()  { echo -e "${BLUE}[阶段2]${NC} $*"; }
ok()    { echo -e "${GREEN}[✓]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
fail()  { echo -e "${RED}[✗]${NC} $*"; exit 1; }

echo -e "${CYAN}╔══════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  阶段 2: 环境安装                     ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════╝${NC}"
echo ""

# ===== 检查 root =====
if [ "$EUID" -ne 0 ]; then
    fail "请使用 sudo 或 root 用户执行"
fi

# ===== 检测发行版 =====
. /etc/os-release
case "$ID" in
    arch|manjaro|endeavouros) DISTRO_FAMILY="arch" ;;
    debian|ubuntu|linuxmint|pop) DISTRO_FAMILY="debian" ;;
    *) fail "不支持: $ID" ;;
esac

# ===== 安装 Docker =====
if command -v docker &>/dev/null; then
    ok "Docker 已安装: $(docker --version)"
else
    info "安装 Docker..."
    case "$DISTRO_FAMILY" in
        arch)
            pacman -S --noconfirm --needed docker docker-compose docker-buildx
            ;;
        debian)
            install -m 0755 -d /etc/apt/keyrings
            curl -fsSL "https://download.docker.com/linux/${ID}/gpg" | \
                gpg --dearmor -o /etc/apt/keyrings/docker.gpg
            chmod a+r /etc/apt/keyrings/docker.gpg

            echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] \
                https://download.docker.com/linux/${ID} $(lsb_release -cs) stable" | \
                tee /etc/apt/sources.list.d/docker.list > /dev/null

            apt-get update
            apt-get install -y docker-ce docker-ce-cli containerd.io \
                docker-buildx-plugin docker-compose-plugin
            ;;
    esac
    ok "Docker 安装完成"
fi

# ===== 启动 Docker =====
systemctl enable --now docker
ok "Docker 服务已启动"

# ===== 加入 docker 组 =====
REAL_USER="${SUDO_USER:-$USER}"
if ! groups "$REAL_USER" 2>/dev/null | grep -q docker; then
    usermod -aG docker "$REAL_USER"
    warn "已将 $REAL_USER 加入 docker 组，重新登录后生效"
    warn "或运行: newgrp docker"
else
    ok "$REAL_USER 已在 docker 组中"
fi

# ===== 验证 Docker Compose =====
if docker compose version &>/dev/null; then
    ok "Docker Compose (v2 插件): $(docker compose version --short)"
elif command -v docker-compose &>/dev/null; then
    ok "Docker Compose (独立版): $(docker-compose --version)"
else
    fail "Docker Compose 未安装"
fi

echo ""
ok "阶段 2 完成 ✓"
echo -e "  下一步: ${BLUE}sudo bash scripts/deploy/stage3-code.sh${NC}"
