#!/usr/bin/env bash
# ============================================================
# Yachiyo 部署 — 阶段 1: 系统准备
# 检查硬件、更新系统、安装基础工具、配置防火墙
# 用法: sudo bash scripts/deploy/stage1-prepare.sh
# ============================================================

set -euo pipefail

# 颜色
GREEN="\033[0;32m"; YELLOW="\033[1;33m"; RED="\033[0;31m"; BLUE="\033[0;34m"; CYAN="\033[0;36m"; NC="\033[0m"
info()  { echo -e "${BLUE}[阶段1]${NC} $*"; }
ok()    { echo -e "${GREEN}[✓]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
fail()  { echo -e "${RED}[✗]${NC} $*"; exit 1; }

echo -e "${CYAN}╔══════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  阶段 1: 系统准备                     ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════╝${NC}"
echo ""

# ===== 检查 root =====
if [ "$EUID" -ne 0 ]; then
    fail "请使用 sudo 或 root 用户执行"
fi

# ===== 检测发行版 =====
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO_ID="${ID}"
else
    fail "无法识别的发行版"
fi

case "$DISTRO_ID" in
    arch|manjaro|endeavouros) DISTRO_FAMILY="arch" ;;
    debian|ubuntu|linuxmint|pop) DISTRO_FAMILY="debian" ;;
    *) fail "不支持: $DISTRO_ID (仅支持 Arch/Debian/Ubuntu 系列)" ;;
esac
ok "系统: ${PRETTY_NAME} (${DISTRO_FAMILY} 系列)"

# ===== 检查硬件 =====
CPU_CORES=$(nproc)
RAM_MB=$(free -m | awk '/Mem:/ {print $2}')
DISK_GB=$(df -BG / | awk 'NR==2 {gsub(/G/,""); print $4}')

info "CPU: ${CPU_CORES} 核 | 内存: ${RAM_MB} MB | 可用磁盘: ${DISK_GB} GB"

WARNINGS=0
[ "$CPU_CORES" -lt 2 ] && { warn "CPU 不足 2 核，编译会非常慢"; WARNINGS=$((WARNINGS+1)); }
[ "$RAM_MB" -lt 3500 ] && { warn "内存不足 4GB，Docker 编译可能 OOM"; WARNINGS=$((WARNINGS+1)); }
[ "$DISK_GB" -lt 15 ] && { warn "磁盘不足 15GB，空间可能不够"; WARNINGS=$((WARNINGS+1)); }

if [ $WARNINGS -eq 0 ]; then
    ok "硬件检查全部通过"
else
    warn "有 $WARNINGS 项警告，建议升级后再部署"
fi

# ===== 更新系统 =====
info "更新系统包..."
case "$DISTRO_FAMILY" in
    arch)   pacman -Syu --noconfirm ;;
    debian) apt-get update -y && apt-get upgrade -y ;;
esac
ok "系统更新完成"

# ===== 安装基础工具 =====
info "安装基础工具..."
case "$DISTRO_FAMILY" in
    arch)
        pacman -S --noconfirm --needed base-devel git curl wget openssl ca-certificates
        ;;
    debian)
        apt-get install -y build-essential git curl wget openssl ca-certificates \
            apt-transport-https gnupg lsb-release software-properties-common
        ;;
esac
ok "基础工具安装完成"

# ===== 配置防火墙 =====
info "配置防火墙..."
case "$DISTRO_FAMILY" in
    debian)
        if ! command -v ufw &>/dev/null; then
            apt-get install -y ufw
        fi
        ufw --force reset
        ufw default deny incoming
        ufw default allow outgoing
        ufw allow 22/tcp
        ufw allow 80/tcp
        ufw allow 443/tcp
        ufw --force enable
        ok "UFW 防火墙已配置 (开放 22/80/443)"
        ;;
    arch)
        if command -v iptables &>/dev/null; then
            iptables -F
            iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
            iptables -A INPUT -i lo -j ACCEPT
            iptables -A INPUT -p tcp --dport 22 -j ACCEPT
            iptables -A INPUT -p tcp --dport 80 -j ACCEPT
            iptables -A INPUT -p tcp --dport 443 -j ACCEPT
            iptables -P INPUT DROP
            mkdir -p /etc/iptables
            iptables-save > /etc/iptables/iptables.rules
            systemctl enable iptables 2>/dev/null || true
            ok "iptables 防火墙已配置 (开放 22/80/443)"
        else
            warn "iptables 未安装，请手动配置防火墙"
        fi
        ;;
esac

echo ""
ok "阶段 1 完成 ✓"
echo -e "  下一步: ${BLUE}sudo bash scripts/deploy/stage2-install.sh${NC}"
