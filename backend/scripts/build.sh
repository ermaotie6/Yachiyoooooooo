#!/bin/bash

# YachiyoCPP 构建脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 函数：打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 函数：检查命令是否存在
check_command() {
    if ! command -v $1 &> /dev/null; then
        print_error "命令 '$1' 未找到，请先安装"
        exit 1
    fi
}

# 函数：检查依赖
check_dependencies() {
    print_info "检查依赖..."
    
    # 检查 CMake
    check_command cmake
    
    # 检查编译器
    if command -v g++ &> /dev/null; then
        print_info "找到 GCC 编译器: $(g++ --version | head -n1)"
    elif command -v clang++ &> /dev/null; then
        print_info "找到 Clang 编译器: $(clang++ --version | head -n1)"
    else
        print_error "未找到 C++ 编译器，请安装 GCC 或 Clang"
        exit 1
    fi
    
    # 检查 make 或 ninja
    if command -v ninja &> /dev/null; then
        print_info "找到 Ninja 构建系统"
        GENERATOR="Ninja"
    elif command -v make &> /dev/null; then
        print_info "找到 Make 构建系统"
        GENERATOR="Unix Makefiles"
    else
        print_error "未找到构建系统，请安装 make 或 ninja"
        exit 1
    fi
    
    # 检查其他工具
    check_command git
    check_command pkg-config
}

# 函数：创建构建目录
create_build_dir() {
    local BUILD_TYPE=$1
    local BUILD_DIR="build_${BUILD_TYPE}"
    
    if [ -d "$BUILD_DIR" ]; then
        print_warning "构建目录 $BUILD_DIR 已存在，正在清理..."
        rm -rf "$BUILD_DIR"
    fi
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    print_info "配置 ${BUILD_TYPE} 构建..."
    
    if [ "$GENERATOR" = "Ninja" ]; then
        cmake .. -G Ninja -DCMAKE_BUILD_TYPE=$BUILD_TYPE
    else
        cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
    fi
    
    if [ $? -ne 0 ]; then
        print_error "CMake 配置失败"
        exit 1
    fi
    
    cd ..
}

# 函数：编译项目
compile_project() {
    local BUILD_TYPE=$1
    local BUILD_DIR="build_${BUILD_TYPE}"
    local THREADS=$(nproc)
    
    print_info "使用 $THREADS 个线程编译 ${BUILD_TYPE} 版本..."
    
    cd "$BUILD_DIR"
    
    if [ "$GENERATOR" = "Ninja" ]; then
        ninja -j$THREADS
    else
        make -j$THREADS
    fi
    
    if [ $? -ne 0 ]; then
        print_error "编译失败"
        exit 1
    fi
    
    cd ..
    print_success "${BUILD_TYPE} 版本编译完成"
}

# 函数：运行测试
run_tests() {
    local BUILD_TYPE=$1
    local BUILD_DIR="build_${BUILD_TYPE}"
    
    if [ ! -f "$BUILD_DIR/tests/unit_tests" ]; then
        print_warning "未找到测试可执行文件，跳过测试"
        return
    fi
    
    print_info "运行单元测试..."
    cd "$BUILD_DIR"
    ./tests/unit_tests
    
    if [ $? -ne 0 ]; then
        print_error "测试失败"
        exit 1
    fi
    
    cd ..
    print_success "所有测试通过"
}

# 函数：安装项目
install_project() {
    local BUILD_TYPE=$1
    local BUILD_DIR="build_${BUILD_TYPE}"
    
    print_info "安装 ${BUILD_TYPE} 版本..."
    
    cd "$BUILD_DIR"
    
    if [ "$GENERATOR" = "Ninja" ]; then
        sudo ninja install
    else
        sudo make install
    fi
    
    if [ $? -ne 0 ]; then
        print_error "安装失败"
        exit 1
    fi
    
    cd ..
    print_success "安装完成"
}

# 函数：清理构建
clean_build() {
    print_info "清理构建文件..."
    
    if [ -d "build_Debug" ]; then
        rm -rf build_Debug
    fi
    
    if [ -d "build_Release" ]; then
        rm -rf build_Release
    fi
    
    if [ -d "build_RelWithDebInfo" ]; then
        rm -rf build_RelWithDebInfo
    fi
    
    print_success "清理完成"
}

# 函数：显示帮助
show_help() {
    echo "YachiyoCPP 构建脚本"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  debug         构建调试版本"
    echo "  release       构建发布版本 (默认)"
    echo "  relwithdebinfo 构建带调试信息的发布版本"
    echo "  all           构建所有版本"
    echo "  test          运行测试"
    echo "  install       安装项目"
    echo "  clean         清理构建文件"
    echo "  help          显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0 debug      构建调试版本"
    echo "  $0 release    构建发布版本"
    echo "  $0 all test   构建所有版本并运行测试"
}

# 主函数
main() {
    # 默认构建类型
    BUILD_TYPE="Release"
    DO_TEST=false
    DO_INSTALL=false
    DO_CLEAN=false
    DO_ALL=false
    
    # 解析参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            debug)
                BUILD_TYPE="Debug"
                shift
                ;;
            release)
                BUILD_TYPE="Release"
                shift
                ;;
            relwithdebinfo)
                BUILD_TYPE="RelWithDebInfo"
                shift
                ;;
            all)
                DO_ALL=true
                shift
                ;;
            test)
                DO_TEST=true
                shift
                ;;
            install)
                DO_INSTALL=true
                shift
                ;;
            clean)
                DO_CLEAN=true
                shift
                ;;
            help|-h|--help)
                show_help
                exit 0
                ;;
            *)
                print_error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # 检查依赖
    check_dependencies
    
    # 清理
    if [ "$DO_CLEAN" = true ]; then
        clean_build
        exit 0
    fi
    
    # 构建所有版本
    if [ "$DO_ALL" = true ]; then
        print_info "开始构建所有版本..."
        
        # 构建调试版本
        create_build_dir "Debug"
        compile_project "Debug"
        
        # 构建发布版本
        create_build_dir "Release"
        compile_project "Release"
        
        # 构建带调试信息的发布版本
        create_build_dir "RelWithDebInfo"
        compile_project "RelWithDebInfo"
        
        print_success "所有版本构建完成"
        
        # 运行测试
        if [ "$DO_TEST" = true ]; then
            run_tests "Debug"
        fi
        
        # 安装
        if [ "$DO_INSTALL" = true ]; then
            install_project "Release"
        fi
        
        exit 0
    fi
    
    # 单个版本构建
    print_info "开始构建 ${BUILD_TYPE} 版本..."
    
    create_build_dir "$BUILD_TYPE"
    compile_project "$BUILD_TYPE"
    
    # 运行测试
    if [ "$DO_TEST" = true ]; then
        run_tests "$BUILD_TYPE"
    fi
    
    # 安装
    if [ "$DO_INSTALL" = true ]; then
        install_project "$BUILD_TYPE"
    fi
    
    print_success "构建完成"
}

# 运行主函数
main "$@"