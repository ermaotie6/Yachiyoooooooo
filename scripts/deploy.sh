#!/bin/bash

# Yachiyo 部署脚本
# 用法: ./scripts/deploy.sh [environment] [version]
# 例: ./scripts/deploy.sh production v2.0.0

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 默认值
ENVIRONMENT="${1:-production}"
VERSION="${2:-latest}"
DOCKER_REGISTRY="${DOCKER_REGISTRY:-docker.io}"
DOCKER_NAMESPACE="${DOCKER_NAMESPACE:-yachiyo}"
DEPLOY_DIR="${DEPLOY_DIR:-/opt/yachiyo}"

# 日志文件
LOG_FILE="$PROJECT_ROOT/yachiyo-deploy.log"

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

log_info "Yachiyo 部署脚本"
log_info "环境: $ENVIRONMENT"
log_info "版本: $VERSION"
log_info "项目根目录: $PROJECT_ROOT"
log_info "部署目录: $DEPLOY_DIR"

# 检查前提条件
check_prerequisites() {
    log_info "检查前提条件..."
    
    local missing_tools=()
    
    if ! command -v docker &> /dev/null; then
        missing_tools+=("docker")
    fi
    
    if ! command -v docker-compose &> /dev/null; then
        missing_tools+=("docker-compose")
    fi
    
    if ! command -v git &> /dev/null; then
        missing_tools+=("git")
    fi
    
    if [ ${#missing_tools[@]} -gt 0 ]; then
        log_error "以下工具未安装: ${missing_tools[*]}"
        return 1
    fi
    
    log_success "所有前提条件已满足"
    return 0
}

# 构建 Docker 镜像
build_images() {
    log_info "构建 Docker 镜像..."
    
    cd "$PROJECT_ROOT"
    
    # 构建后端镜像
    log_info "构建后端镜像..."
    docker build \
        -f backend/Dockerfile \
        -t "$DOCKER_REGISTRY/$DOCKER_NAMESPACE/backend:$VERSION" \
        -t "$DOCKER_REGISTRY/$DOCKER_NAMESPACE/backend:latest" \
        backend/
    
    if [ $? -ne 0 ]; then
        log_error "后端镜像构建失败"
        return 1
    fi
    
    log_success "后端镜像构建成功"
    
    # 构建前端镜像
    log_info "构建前端镜像..."
    docker build \
        -f frontend/Dockerfile \
        -t "$DOCKER_REGISTRY/$DOCKER_NAMESPACE/frontend:$VERSION" \
        -t "$DOCKER_REGISTRY/$DOCKER_NAMESPACE/frontend:latest" \
        frontend/
    
    if [ $? -ne 0 ]; then
        log_error "前端镜像构建失败"
        return 1
    fi
    
    log_success "前端镜像构建成功"
    
    return 0
}

# 推送镜像到仓库
push_images() {
    if [ "$DOCKER_REGISTRY" = "docker.io" ] || [ -z "$DOCKER_REGISTRY" ]; then
        log_warning "跳过推送镜像 (使用本地仓库)"
        return 0
    fi
    
    log_info "推送镜像到仓库..."
    
    # 推送后端镜像
    log_info "推送后端镜像..."
    docker push "$DOCKER_REGISTRY/$DOCKER_NAMESPACE/backend:$VERSION"
    docker push "$DOCKER_REGISTRY/$DOCKER_NAMESPACE/backend:latest"
    
    # 推送前端镜像
    log_info "推送前端镜像..."
    docker push "$DOCKER_REGISTRY/$DOCKER_NAMESPACE/frontend:$VERSION"
    docker push "$DOCKER_REGISTRY/$DOCKER_NAMESPACE/frontend:latest"
    
    log_success "镜像推送完成"
    return 0
}

# 准备部署目录
prepare_deploy_dir() {
    log_info "准备部署目录..."
    
    if [ ! -d "$DEPLOY_DIR" ]; then
        log_info "创建部署目录: $DEPLOY_DIR"
        sudo mkdir -p "$DEPLOY_DIR"
    fi
    
    # 备份现有配置
    if [ -d "$DEPLOY_DIR/config" ]; then
        log_info "备份现有配置..."
        sudo cp -r "$DEPLOY_DIR/config" "$DEPLOY_DIR/config.backup.$(date +%Y%m%d_%H%M%S)"
    fi
    
    # 复制配置文件
    log_info "复制配置文件..."
    sudo cp -r "$PROJECT_ROOT/backend/config" "$DEPLOY_DIR/" 2>/dev/null || true
    
    # 复制 Docker Compose 文件
    log_info "复制 Docker Compose 文件..."
    sudo cp "$PROJECT_ROOT/docker-compose.yml" "$DEPLOY_DIR/"
    
    # 复制环境变量文件
    if [ -f "$PROJECT_ROOT/.env" ]; then
        sudo cp "$PROJECT_ROOT/.env" "$DEPLOY_DIR/.env"
    fi
    
    log_success "部署目录准备完成"
    return 0
}

# 更新生产服务
update_production() {
    log_info "更新生产服务..."
    
    if [ ! -d "$DEPLOY_DIR" ]; then
        log_error "部署目录不存在: $DEPLOY_DIR"
        return 1
    fi
    
    cd "$DEPLOY_DIR"
    
    # 停止现有服务
    log_info "停止现有服务..."
    sudo docker-compose down || true
    
    # 更新镜像版本 (如果需要)
    if [ "$VERSION" != "latest" ]; then
        log_info "更新 Docker Compose 中的镜像版本..."
        sudo sed -i "s|backend:latest|backend:$VERSION|g" docker-compose.yml
        sudo sed -i "s|frontend:latest|frontend:$VERSION|g" docker-compose.yml
    fi
    
    # 启动新服务
    log_info "启动新服务..."
    sudo docker-compose up -d
    
    # 等待服务启动
    log_info "等待服务启动..."
    sleep 10
    
    # 检查服务状态
    log_info "检查服务状态..."
    sudo docker-compose ps
    
    log_success "生产服务更新完成"
    return 0
}

# 运行健康检查
health_check() {
    log_info "运行健康检查..."
    
    local max_attempts=30
    local attempt=1
    
    while [ $attempt -le $max_attempts ]; do
        if curl -f http://localhost:8080/api/v1/health > /dev/null 2>&1; then
            log_success "后端健康检查通过"
            break
        fi
        
        log_warning "健康检查失败，重试 ($attempt/$max_attempts)..."
        sleep 2
        ((attempt++))
    done
    
    if [ $attempt -gt $max_attempts ]; then
        log_error "健康检查超时"
        return 1
    fi
    
    # 检查前端
    if curl -f http://localhost:3000 > /dev/null 2>&1; then
        log_success "前端健康检查通过"
    else
        log_warning "前端健康检查失败，但继续部署"
    fi
    
    return 0
}

# 显示部署信息
show_deploy_info() {
    log_success "========================================="
    log_success "Yachiyo 部署完成！"
    log_success "========================================="
    
    log_info ""
    log_info "📍 服务地址:"
    log_info "   前端:        http://localhost:3000"
    log_info "   后端 API:    http://localhost:8080/api/v1"
    log_info "   WebSocket:   ws://localhost:8081"
    log_info ""
    log_info "📁 部署目录:      $DEPLOY_DIR"
    log_info "📋 镜像版本:      $VERSION"
    log_info "🌍 环境:         $ENVIRONMENT"
    log_info ""
    log_info "📝 Docker 命令:"
    log_info "   查看日志:     docker-compose logs -f backend"
    log_info "   重启服务:     docker-compose restart"
    log_info "   停止服务:     docker-compose down"
    log_info "   更新镜像:     docker-compose pull && docker-compose up -d"
    log_info ""
    log_success "========================================="
}

# 显示帮助信息
show_help() {
    cat << EOF
Yachiyo 部署脚本

用法: $0 [environment] [version]

参数:
  environment   部署环境 (development/production), 默认: production
  version       镜像版本标签, 默认: latest

环境变量:
  DOCKER_REGISTRY    Docker 仓库地址, 默认: docker.io
  DOCKER_NAMESPACE   Docker 命名空间, 默认: yachiyo
  DEPLOY_DIR         部署目录, 默认: /opt/yachiyo

例子:
  默认部署 (生产环境, latest):
    $0

  指定版本部署:
    $0 production v2.0.0

  开发环境部署:
    $0 development

  使用自定义仓库:
    DOCKER_REGISTRY=registry.company.com DOCKER_NAMESPACE=myapp $0 production v1.0.0

说明:
  - 脚本会自动构建、推送镜像并更新服务
  - 需要 Docker、Docker Compose 和 git
  - 需要 sudo 权限来修改 /opt/yachiyo 目录
  - 首次部署建议使用 'development' 环境进行测试

EOF
}

# 主函数
main() {
    if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
        show_help
        return 0
    fi
    
    log_info "开始部署 Yachiyo..."
    
    # 检查前提条件
    if ! check_prerequisites; then
        log_error "前提条件检查失败"
        return 1
    fi
    
    # 构建镜像
    if ! build_images; then
        log_error "构建镜像失败"
        return 1
    fi
    
    # 推送镜像 (可选)
    if ! push_images; then
        log_warning "推送镜像失败，继续部署..."
    fi
    
    # 准备部署目录
    if ! prepare_deploy_dir; then
        log_error "准备部署目录失败"
        return 1
    fi
    
    # 更新生产服务
    if ! update_production; then
        log_error "更新生产服务失败"
        return 1
    fi
    
    # 运行健康检查
    if ! health_check; then
        log_error "健康检查失败，部署可能有问题"
        return 1
    fi
    
    # 显示部署信息
    show_deploy_info
}

# 执行主函数
main "$@"
exit $?
