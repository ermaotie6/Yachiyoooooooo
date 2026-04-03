#!/bin/bash

# Yachiyo 启动脚本
# 用法: ./scripts/start.sh [environment] [mode]
# 例: ./scripts/start.sh production docker

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 默认值
ENVIRONMENT="${1:-development}"
MODE="${2:-local}"
DOCKER_COMPOSE_FILE="$PROJECT_ROOT/docker-compose.yml"
BACKEND_BUILD_DIR="$PROJECT_ROOT/backend/build"
FRONTEND_DIR="$PROJECT_ROOT/frontend"

# 日志文件
LOG_FILE="$PROJECT_ROOT/yachiyo-startup.log"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

log_info "Yachiyo 启动脚本"
log_info "环境: $ENVIRONMENT"
log_info "模式: $MODE"
log_info "项目根目录: $PROJECT_ROOT"

# 检查环境变量文件
check_env_files() {
    log_info "检查环境变量文件..."
    
    if [ ! -f "$PROJECT_ROOT/backend/.env" ]; then
        if [ -f "$PROJECT_ROOT/backend/.env.example" ]; then
            log_warning "后端 .env 文件不存在，正在复制 .env.example"
            cp "$PROJECT_ROOT/backend/.env.example" "$PROJECT_ROOT/backend/.env"
        else
            log_error "后端 .env.example 文件不存在"
            return 1
        fi
    fi
    
    if [ ! -f "$PROJECT_ROOT/frontend/.env.local" ]; then
        if [ -f "$PROJECT_ROOT/frontend/.env.example" ]; then
            log_warning "前端 .env.local 文件不存在，正在复制 .env.example"
            cp "$PROJECT_ROOT/frontend/.env.example" "$PROJECT_ROOT/frontend/.env.local"
        else
            log_error "前端 .env.example 文件不存在"
            return 1
        fi
    fi
    
    log_success "环境变量文件检查完成"
    return 0
}

# Docker 模式启动
start_docker() {
    log_info "使用 Docker Compose 启动服务..."
    
    if ! command -v docker-compose &> /dev/null; then
        log_error "Docker Compose 未安装，请先安装"
        return 1
    fi
    
    cd "$PROJECT_ROOT"
    
    if [ "$ENVIRONMENT" = "production" ]; then
        log_info "生产环境启动..."
        docker-compose -f "$DOCKER_COMPOSE_FILE" up -d
    else
        log_info "开发环境启动..."
        docker-compose -f "$DOCKER_COMPOSE_FILE" up -d
    fi
    
    log_success "Docker 服务启动成功"
    
    # 等待服务启动
    log_info "等待服务启动..."
    sleep 5
    
    # 检查服务状态
    docker-compose ps
    
    return 0
}

# 本地模式启动
start_local() {
    log_info "本地模式启动..."
    
    # 检查数据库
    if ! command -v psql &> /dev/null; then
        log_error "PostgreSQL 未安装，请先安装"
        return 1
    fi
    
    # 启动后端
    log_info "启动后端服务..."
    if [ ! -d "$BACKEND_BUILD_DIR" ]; then
        log_error "后端未编译，请先运行: cd backend && mkdir build && cd build && cmake .. && make"
        return 1
    fi
    
    if [ ! -f "$BACKEND_BUILD_DIR/src/yachiyo_cpp" ]; then
        log_error "后端可执行文件不存在，请先编译"
        return 1
    fi
    
    # 在后台启动后端
    nohup "$BACKEND_BUILD_DIR/src/yachiyo_cpp" \
        --config-dir "$PROJECT_ROOT/backend/config" \
        --env "$ENVIRONMENT" \
        --port 8080 \
        > "$PROJECT_ROOT/backend.log" 2>&1 &
    
    BACKEND_PID=$!
    log_success "后端服务已启动 (PID: $BACKEND_PID)"
    
    # 启动前端
    log_info "启动前端服务..."
    cd "$FRONTEND_DIR"
    
    if [ ! -d "node_modules" ]; then
        log_warning "前端依赖未安装，正在安装..."
        npm install
    fi
    
    nohup npm run dev > "$PROJECT_ROOT/frontend.log" 2>&1 &
    
    FRONTEND_PID=$!
    log_success "前端服务已启动 (PID: $FRONTEND_PID)"
    
    # 保存 PID 以便后续停止
    echo "$BACKEND_PID" > "$PROJECT_ROOT/.backend.pid"
    echo "$FRONTEND_PID" > "$PROJECT_ROOT/.frontend.pid"
    
    return 0
}

# 显示启动信息
show_startup_info() {
    log_success "========================================="
    log_success "Yachiyo 启动完成！"
    log_success "========================================="
    
    if [ "$MODE" = "docker" ]; then
        log_info ""
        log_info "📍 服务地址:"
        log_info "   前端:        http://localhost:3000"
        log_info "   后端 API:    http://localhost:8080/api/v1"
        log_info "   WebSocket:   ws://localhost:8081"
        log_info "   PostgreSQL:  localhost:5432 (postgres:postgres)"
        log_info "   Redis:       localhost:6379"
        log_info "   pgAdmin:     http://localhost:5050"
        log_info "   Grafana:     http://localhost:3001"
        log_info ""
        log_info "📋 常用命令:"
        log_info "   查看日志:    docker-compose logs -f backend"
        log_info "   停止服务:    docker-compose down"
        log_info "   重启服务:    docker-compose restart backend"
    else
        log_info ""
        log_info "📍 服务地址:"
        log_info "   前端:        http://localhost:5173 (Vite 开发服务器)"
        log_info "   后端 API:    http://localhost:8080/api/v1"
        log_info "   WebSocket:   ws://localhost:8081"
        log_info ""
        log_info "📋 日志文件:"
        log_info "   后端日志:    $PROJECT_ROOT/backend.log"
        log_info "   前端日志:    $PROJECT_ROOT/frontend.log"
        log_info ""
        log_info "🛑 停止服务:"
        log_info "   运行: ./scripts/stop.sh"
    fi
    
    log_info ""
    log_info "📚 文档: ./FULL_README.md"
    log_info "❓ 帮助: ./scripts/start.sh --help"
    log_success "========================================="
}

# 显示帮助信息
show_help() {
    cat << EOF
Yachiyo 启动脚本

用法: $0 [environment] [mode]

参数:
  environment   运行环境 (development/production), 默认: development
  mode          启动模式 (local/docker), 默认: local

例子:
  启动开发环境 (本地):
    $0 development local

  启动生产环境 (Docker):
    $0 production docker

  启动开发环境 (Docker):
    $0 development docker

说明:
  - local 模式: 在本机运行后端和前端，需要本地安装 C++/Node.js 编译工具
  - docker 模式: 使用 Docker Compose 启动所有服务，更推荐

EOF
}

# 主函数
main() {
    if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
        show_help
        return 0
    fi
    
    log_info "开始启动 Yachiyo..."
    
    # 检查环境变量文件
    if ! check_env_files; then
        log_error "环境变量检查失败"
        return 1
    fi
    
    # 根据模式启动
    case "$MODE" in
        docker)
            if ! start_docker; then
                log_error "Docker 启动失败"
                return 1
            fi
            ;;
        local)
            if ! start_local; then
                log_error "本地启动失败"
                return 1
            fi
            ;;
        *)
            log_error "未知的启动模式: $MODE (有效值: local, docker)"
            return 1
            ;;
    esac
    
    # 显示启动信息
    show_startup_info
}

# 执行主函数
main "$@"
exit $?
