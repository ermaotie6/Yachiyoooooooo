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
print_color() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# 函数：检查命令是否存在
check_command() {
    if ! command -v $1 &> /dev/null; then
        print_color $RED "错误: $1 未安装"
        exit 1
    fi
}

# 函数：检查依赖
check_dependencies() {
    print_color $BLUE "检查依赖..."
    
    # 检查CMake
    check_command cmake
    
    # 检查C++编译器
    if command -v g++ &> /dev/null; then
        print_color $GREEN "找到 g++"
    elif command -v clang++ &> /dev/null; then
        print_color $GREEN "找到 clang++"
    else
        print_color $RED "错误: 未找到C++编译器 (g++ 或 clang++)"
        exit 1
    fi
    
    # 检查make或ninja
    if command -v make &> /dev/null; then
        print_color $GREEN "找到 make"
    elif command -v ninja &> /dev/null; then
        print_color $GREEN "找到 ninja"
    else
        print_color $RED "错误: 未找到构建工具 (make 或 ninja)"
        exit 1
    fi
}

# 函数：清理构建目录
clean_build() {
    print_color $YELLOW "清理构建目录..."
    rm -rf build
    print_color $GREEN "清理完成"
}

# 函数：配置项目
configure_project() {
    print_color $BLUE "配置项目..."
    
    mkdir -p build
    cd build
    
    # 选择生成器
    if command -v ninja &> /dev/null; then
        GENERATOR="Ninja"
    else
        GENERATOR="Unix Makefiles"
    fi
    
    # 运行CMake
    cmake .. -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_STANDARD=20 \
        -DCMAKE_CXX_STANDARD_REQUIRED=ON \
        -DCMAKE_CXX_EXTENSIONS=OFF
    
    if [ $? -eq 0 ]; then
        print_color $GREEN "配置成功"
    else
        print_color $RED "配置失败"
        exit 1
    fi
    
    cd ..
}

# 函数：构建项目
build_project() {
    print_color $BLUE "构建项目..."
    
    cd build
    
    # 获取CPU核心数
    if command -v nproc &> /dev/null; then
        CORES=$(nproc)
    else
        CORES=4
    fi
    
    # 构建
    if command -v ninja &> /dev/null; then
        ninja -j$CORES
    else
        make -j$CORES
    fi
    
    if [ $? -eq 0 ]; then
        print_color $GREEN "构建成功"
    else
        print_color $RED "构建失败"
        exit 1
    fi
    
    cd ..
}

# 函数：运行测试
run_tests() {
    print_color $BLUE "运行测试..."
    
    cd build
    
    if [ -f "./tests/yachiyo_tests" ]; then
        ./tests/yachiyo_tests
        if [ $? -eq 0 ]; then
            print_color $GREEN "测试通过"
        else
            print_color $RED "测试失败"
            exit 1
        fi
    else
        print_color $YELLOW "未找到测试可执行文件，跳过测试"
    fi
    
    cd ..
}

# 函数：运行应用程序
run_application() {
    print_color $BLUE "运行应用程序..."
    
    if [ -f "./build/src/yachiyo" ]; then
        print_color $GREEN "启动 Yachiyo 服务器..."
        ./build/src/yachiyo "$@"
    else
        print_color $RED "错误: 未找到可执行文件"
        exit 1
    fi
}

# 函数：安装依赖
install_dependencies() {
    print_color $BLUE "安装系统依赖..."
    
    # 检测操作系统
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS=$ID
    else
        print_color $RED "无法检测操作系统"
        exit 1
    fi
    
    case $OS in
        ubuntu|debian)
            sudo apt-get update
            sudo apt-get install -y \
                build-essential \
                cmake \
                ninja-build \
                libssl-dev \
                libyaml-cpp-dev \
                libmysqlclient-dev \
                libhiredis-dev \
                libcurl4-openssl-dev \
                nlohmann-json3-dev
            ;;
        fedora|centos|rhel)
            sudo dnf install -y \
                gcc-c++ \
                cmake \
                ninja-build \
                openssl-devel \
                yaml-cpp-devel \
                mysql-devel \
                hiredis-devel \
                libcurl-devel \
                nlohmann-json-devel
            ;;
        arch|manjaro)
            sudo pacman -S --noconfirm \
                base-devel \
                cmake \
                ninja \
                openssl \
                yaml-cpp \
                mysql-connector-c \
                hiredis \
                curl \
                nlohmann-json
            ;;
        *)
            print_color $RED "不支持的操作系统: $OS"
            print_color $YELLOW "请手动安装以下依赖:"
            print_color $YELLOW "  - C++编译器 (g++ 或 clang++)"
            print_color $YELLOW "  - CMake"
            print_color $YELLOW "  - OpenSSL"
            print_color $YELLOW "  - yaml-cpp"
            print_color $YELLOW "  - MySQL客户端库"
            print_color $YELLOW "  - hiredis"
            print_color $YELLOW "  - libcurl"
            print_color $YELLOW "  - nlohmann-json"
            exit 1
            ;;
    esac
    
    print_color $GREEN "依赖安装完成"
}

# 函数：显示帮助
show_help() {
    echo "YachiyoCPP 构建脚本"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  clean       清理构建目录"
    echo "  configure   配置项目"
    echo "  build       构建项目"
    echo "  test        运行测试"
    echo "  run         运行应用程序"
    echo "  install     安装系统依赖"
    echo "  all         执行所有步骤 (默认)"
    echo "  help        显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0 clean        # 清理"
    echo "  $0 build        # 构建"
    echo "  $0 run          # 运行"
    echo "  $0 all          # 完整构建并运行"
}

# 主函数
main() {
    print_color $BLUE "========================================"
    print_color $BLUE "    YachiyoCPP 构建系统"
    print_color $BLUE "========================================"
    
    # 检查依赖
    check_dependencies
    
    # 解析参数
    if [ $# -eq 0 ]; then
        ACTION="all"
    else
        ACTION=$1
        shift
    fi
    
    case $ACTION in
        clean)
            clean_build
            ;;
        configure)
            configure_project
            ;;
        build)
            configure_project
            build_project
            ;;
        test)
            run_tests
            ;;
        run)
            run_application "$@"
            ;;
        install)
            install_dependencies
            ;;
        all)
            clean_build
            configure_project
            build_project
            run_tests
            print_color $GREEN "所有步骤完成"
            ;;
        help)
            show_help
            ;;
        *)
            print_color $RED "未知操作: $ACTION"
            show_help
            exit 1
            ;;
    esac
}

# 运行主函数
main "$@"