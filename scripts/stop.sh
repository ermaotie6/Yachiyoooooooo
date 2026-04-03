#!/bin/bash

# Yachiyo 停止脚本
# 用法: ./scripts/stop.sh [mode]
# 例: ./scripts/stop.sh docker

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 默认值
MODE="${1:-auto}"
DOCKER_COMPOSE_FILE="$PROJECT_ROOT/docker-compose.yml"

# 日志文件
LOG_FILE="$PROJECT_ROOT/yachiyo-shutdown.log"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1" | tee -a "$LOG_FILE"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1" | tee -a "$LOG_FILE"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1" | tee -a "$LOG_FILE"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" | tee -a "$LOG_FILE"
}

# 清空日志文件
> "$LOG_FILE"

log_info "Yachiyo 停止脚本"
log_info "模式: $MODE"

# Docker 模式停止
stop_docker() {
    log_info "停止 Docker Compose 服务..."
    
    if ! command -v docker-compose &> /dev/null; then
        log_error "Docker Compose 未安装"
        return 1
    fi
    
    cd "$PROJECT_ROOT"
    
    log_info "正在停止容器..."
    docker-compose -f "$DOCKER_COMPOSE_FILE" down
    
    log_success "Docker 服务已停止"
    return 0
}

# 本地模式停止
stop_local() {
    log_info "停止本地服务..."
    
    # 停止后端
    if [ -f "$PROJECT_ROOT/.backend.pid" ]; then
        BACKEND_PID=$(cat "$PROJECT_ROOT/.backend.pid")
        if ps -p "$BACKEND_PID" > /dev/null 2>&1; then
            log_info "停止后端服务 (PID: $BACKEND_PID)..."
            kill "$BACKEND_PID" || true
            sleep 2
            if ps -p "$BACKEND_PID" > /dev/null 2>&1; then
                log_warning "强制终止后端服务..."
                kill -9 "$BACKEND_PID" || true
            fi
        fi
        rm -f "$PROJECT_ROOT/.backend.pid"
        log_success "后端服务已停止"
    else
        log_warning "未找到后端 PID 文件"
    fi
    
    # 停止前端
    if [ -f "$PROJECT_ROOT/.frontend.pid" ]; then
        FRONTEND_PID=$(cat "$PROJECT_ROOT/.frontend.pid")
        if ps -p "$FRONTEND_PID" > /dev/null 2>&1; then
            log_info "停止前端服务 (PID: $FRONTEND_PID)..."
            kill "$FRONTEND_PID" || true
            sleep 2
            if ps -p "$FRONTEND_PID" > /dev/null 2>&1; then
                log_warning "强制终止前端服务..."
                kill -9 "$FRONTEND_PID" || true
            fi
        fi
        rm -f "$PROJECT_ROOT/.frontend.pid"
        log_success "前端服务已停止"
    else
        log_warning "未找到前端 PID 文件"
    fi
    
    return 0
}

# 自动检测并停止
stop_auto() {
    log_info "自动检测运行模式..."
    
    # 检查 Docker 容器是否在运行
    if command -v docker-compose &> /dev/null && cd "$PROJECT_ROOT" && docker-compose -f "$DOCKER_COMPOSE_FILE" ps 2>/dev/null | grep -q "Up"; then
        log_info "检测到 Docker Compose 运行，使用 Docker 模式停止"
        stop_docker
    elif [ -f "$PROJECT_ROOT/.backend.pid" ] || [ -f "$PROJECT_ROOT/.frontend.pid" ]; then
        log_info "检测到本地进程，使用本地模式停止"
        stop_local
    else
        log_warning "未检测到运行的 Yachiyo 服务"
        return 0
    fi
}

# 显示帮助信息
show_help() {
    cat << EOF
Yachiyo 停止脚本

用法: $0 [mode]

参数:
  mode   停止模式 (local/docker/auto), 默认: auto

例子:
  自动检测模式停止:
    $0

  使用 Docker 模式停止:
    $0 docker

  使用本地模式停止:
    $0 local

说明:
  - auto 模式: 自动检测当前运行方式并停止
  - local 模式: 停止本地运行的进程
  - docker 模式: 停止 Docker Compose 容器

EOF
}

# 主函数
main() {
    if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
        show_help
        return 0
    fi
    
    log_info "开始停止 Yachiyo..."
    
    case "$MODE" in
        docker)
            if ! stop_docker; then
                log_error "Docker 停止失败"
                return 1
            fi
            ;;
        local)
            if ! stop_local; then
                log_error "本地停止失败"
                return 1
            fi
            ;;
        auto)
            if ! stop_auto; then
                log_error "自动停止失败"
                return 1
            fi
            ;;
        *)
            log_error "未知的停止模式: $MODE (有效值: local, docker, auto)"
            return 1
            ;;
    esac
    
    log_success "========================================="
    log_success "Yachiyo 已停止"
    log_success "========================================="
}

# 执行主函数
main "$@"
exit $?
