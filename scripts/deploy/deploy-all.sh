#!/usr/bin/env bash
# ============================================================
# Yachiyo 全流程部署脚本 — 串联执行阶段 1~7
# 阶段 7 (HTTPS) 现在自动配置 nginx，不再需要手动编辑
#
# 用法:
#   sudo bash scripts/deploy/deploy-all.sh          # 全流程
#   sudo bash scripts/deploy/deploy-all.sh --from 4 # 从阶段 4 开始
#   sudo bash scripts/deploy/deploy-all.sh --only 6 # 只跑阶段 6
#   sudo bash scripts/deploy/deploy-all.sh --skip-ssl # 跳过 HTTPS
# ============================================================

set -euo pipefail

GREEN="\033[0;32m"; YELLOW="\033[1;33m"; RED="\033[0;31m"; BLUE="\033[0;34m"; CYAN="\033[0;36m"; NC="\033[0m"
info()  { echo -e "${BLUE}[部署]${NC} $*"; }
ok()    { echo -e "${GREEN}[✓]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
fail()  { echo -e "${RED}[✗]${NC} $*"; }

# ===== 解析参数 =====
START_STAGE=1
END_STAGE=7
ONLY_STAGE=""
SKIP_SSL=false

while [ $# -gt 0 ]; do
    case "$1" in
        --from)
            START_STAGE="$2"
            shift 2
            ;;
        --only)
            ONLY_STAGE="$2"
            START_STAGE="$2"
            END_STAGE="$2"
            shift 2
            ;;
        --skip-ssl)
            SKIP_SSL=true
            END_STAGE=6
            shift
            ;;
        --help|-h)
            echo "用法: $0 [选项]"
            echo ""
            echo "选项:"
            echo "  --from N   从阶段 N 开始执行 (1~7)"
            echo "  --only N   只执行阶段 N"
            echo "  --skip-ssl 跳过阶段 7 (HTTPS 加固)"
            echo "  --help     显示帮助"
            echo ""
            echo "阶段:"
            echo "  1  系统准备       (更新系统、安装基础工具、防火墙)"
            echo "  2  环境安装       (Docker + Docker Compose)"
            echo "  3  代码部署       (克隆/更新项目代码)"
            echo "  4  密钥配置       (交互式生成 .env)"
            echo "  5  构建启动       (Docker Compose 构建 + 启动)"
            echo "  6  验证测试       (7 项健康检查)"
            echo "  7  HTTPS 加固     (SSL 证书 + 安全检查)"
            exit 0
            ;;
        *)
            echo "未知参数: $1"
            exit 1
            ;;
    esac
done

# ===== 检查 root =====
if [ "$EUID" -ne 0 ]; then
    fail "请使用 sudo 或 root 用户执行"
    exit 1
fi

# ===== 定位脚本目录 =====
# 支持从任意位置调用
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "${CYAN}"
echo "  ╔═══════════════════════════════════════╗"
echo "  ║  🎀 Yachiyo 全流程部署                ║"
echo "  ║  AI 虚拟形象直播平台                  ║"
echo "  ╚═══════════════════════════════════════╝"
echo -e "${NC}"

if [ -n "$ONLY_STAGE" ]; then
    info "只执行阶段 $ONLY_STAGE"
else
    info "执行阶段 $START_STAGE → $END_STAGE"
fi
echo ""

# ===== 阶段映射 =====
declare -A STAGE_SCRIPTS
STAGE_SCRIPTS[1]="$SCRIPT_DIR/stage1-prepare.sh"
STAGE_SCRIPTS[2]="$SCRIPT_DIR/stage2-install.sh"
STAGE_SCRIPTS[3]="$SCRIPT_DIR/stage3-code.sh"
STAGE_SCRIPTS[4]="$SCRIPT_DIR/stage4-config.sh"
STAGE_SCRIPTS[5]="$SCRIPT_DIR/stage5-build.sh"
STAGE_SCRIPTS[6]="$SCRIPT_DIR/stage6-verify.sh"
STAGE_SCRIPTS[7]="$SCRIPT_DIR/stage7-secure.sh"

declare -A STAGE_NAMES
STAGE_NAMES[1]="系统准备"
STAGE_NAMES[2]="环境安装"
STAGE_NAMES[3]="代码部署"
STAGE_NAMES[4]="密钥配置"
STAGE_NAMES[5]="构建启动"
STAGE_NAMES[6]="验证测试"
STAGE_NAMES[7]="HTTPS 加固"

# ===== 执行各阶段 =====
FAILED_STAGE=""

for STAGE in $(seq "$START_STAGE" "$END_STAGE"); do
    SCRIPT="${STAGE_SCRIPTS[$STAGE]}"
    NAME="${STAGE_NAMES[$STAGE]}"

    if [ ! -f "$SCRIPT" ]; then
        fail "阶段 $STAGE 脚本不存在: $SCRIPT"
        exit 1
    fi

    echo ""
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    info "开始执行阶段 $STAGE: $NAME"
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    if bash "$SCRIPT"; then
        ok "阶段 $STAGE ($NAME) 完成"
    else
        EXIT_CODE=$?
        # 阶段 6 (验证) 返回失败数量，不一定要中断
        if [ "$STAGE" -eq 6 ]; then
            warn "阶段 6 有 $EXIT_CODE 项测试失败"
        else
            fail "阶段 $STAGE ($NAME) 失败 (退出码: $EXIT_CODE)"
            FAILED_STAGE="$STAGE"
            echo ""
            warn "部署中断于阶段 $STAGE"
            warn "修复后可从此阶段继续: sudo bash $0 --from $STAGE"
            exit "$EXIT_CODE"
        fi
    fi
done

# ===== 完成 =====
echo ""
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

if [ "$SKIP_SSL" = true ]; then
    ok "阶段 $START_STAGE ~ $END_STAGE 全部完成 ✓"
    echo ""
    echo -e "  稍后可手动执行: ${BLUE}sudo bash $SCRIPT_DIR/stage7-secure.sh${NC}  ← 启用 HTTPS + 安全加固"
elif [ "$END_STAGE" -ge 7 ]; then
    ok "全部 7 个阶段执行完成 ✓"
else
    ok "阶段 $START_STAGE ~ $END_STAGE 全部完成 ✓"
fi

echo ""
echo -e "${GREEN}🎀 Yachiyo 部署完成！${NC}"
