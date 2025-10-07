#!/bin/bash
# WEAK_NET 项目安装脚本
# 自动检测环境并安装依赖

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
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

# 检查是否为root用户
check_root() {
    if [ "$EUID" -eq 0 ]; then
        print_warning "检测到root用户，建议使用普通用户运行此脚本"
        read -p "是否继续? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
}

# 检测操作系统
detect_os() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS=$NAME
        VERSION=$VERSION_ID
    else
        print_error "无法检测操作系统"
        exit 1
    fi
    
    print_info "检测到操作系统: $OS $VERSION"
}

# 安装依赖
install_dependencies() {
    print_info "安装系统依赖..."
    
    case "$OS" in
        "Ubuntu"|"Debian GNU/Linux")
            sudo apt-get update
            sudo apt-get install -y \
                build-essential \
                clang \
                llvm \
                pkg-config \
                libdbus-1-dev \
                libglog-dev \
                libelf-dev \
                zlib1g-dev \
                libcap-dev \
                linux-headers-$(uname -r) \
                libbpf-dev
            ;;
        "CentOS Linux"|"Red Hat Enterprise Linux")
            sudo yum groupinstall -y "Development Tools"
            sudo yum install -y \
                clang \
                llvm \
                pkgconfig \
                dbus-devel \
                glog-devel \
                elfutils-libelf-devel \
                zlib-devel \
                libcap-devel \
                kernel-devel-$(uname -r) \
                libbpf-devel
            ;;
        "Fedora")
            sudo dnf groupinstall -y "Development Tools"
            sudo dnf install -y \
                clang \
                llvm \
                pkgconfig \
                dbus-devel \
                glog-devel \
                elfutils-libelf-devel \
                zlib-devel \
                libcap-devel \
                kernel-devel-$(uname -r) \
                libbpf-devel
            ;;
        *)
            print_warning "未识别的操作系统: $OS"
            print_info "请手动安装以下依赖:"
            print_info "  - build-essential (或 Development Tools)"
            print_info "  - clang, llvm"
            print_info "  - pkg-config"
            print_info "  - libdbus-1-dev (或 dbus-devel)"
            print_info "  - libglog-dev (或 glog-devel)"
            print_info "  - libelf-dev (或 elfutils-libelf-devel)"
            print_info "  - zlib1g-dev (或 zlib-devel)"
            print_info "  - libcap-dev (或 libcap-devel)"
            print_info "  - linux-headers-$(uname -r) (或 kernel-devel)"
            print_info "  - libbpf-dev (或 libbpf-devel)"
            read -p "是否继续编译? (y/N): " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                exit 1
            fi
            ;;
    esac
    
    print_success "依赖安装完成"
}

# 检查依赖
check_dependencies() {
    print_info "检查编译依赖..."
    
    local missing_deps=()
    
    # 检查编译器
    if ! command -v g++ &> /dev/null; then
        missing_deps+=("g++")
    fi
    
    if ! command -v clang &> /dev/null; then
        missing_deps+=("clang")
    fi
    
    # 检查pkg-config
    if ! command -v pkg-config &> /dev/null; then
        missing_deps+=("pkg-config")
    fi
    
    # 检查库文件
    if ! pkg-config --exists dbus-1; then
        missing_deps+=("libdbus-1-dev")
    fi
    
    if ! pkg-config --exists libglog; then
        missing_deps+=("libglog-dev")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "缺少以下依赖: ${missing_deps[*]}"
        print_info "请运行: $0 --install-deps"
        exit 1
    fi
    
    print_success "所有依赖检查通过"
}

# 编译项目
build_project() {
    print_info "编译WEAK_NET项目..."
    
    # 清理之前的编译产物
    make clean 2>/dev/null || true
    
    # 编译
    if make all; then
        print_success "编译成功"
    else
        print_error "编译失败"
        exit 1
    fi
}

# 运行测试
run_tests() {
    print_info "运行基本测试..."
    
    # 检查服务器是否已运行
    if pgrep -f weaknet-dbus-server > /dev/null; then
        print_warning "检测到服务器已在运行，跳过测试"
        return 0
    fi
    
    # 启动服务器进行测试
    print_info "启动服务器进行测试..."
    ./server/bin/weaknet-dbus-server &
    local server_pid=$!
    
    # 等待服务器启动
    sleep 3
    
    # 运行客户端测试
    if make test-client COMMAND=get; then
        print_success "基本功能测试通过"
    else
        print_warning "基本功能测试失败"
    fi
    
    # 停止服务器
    kill $server_pid 2>/dev/null || true
    sleep 1
}

# 创建启动脚本
create_scripts() {
    print_info "创建启动脚本..."
    
    # 创建服务器启动脚本
    cat > start-server.sh << 'EOF'
#!/bin/bash
# WEAK_NET 服务器启动脚本

cd "$(dirname "$0")"

# 检查依赖
if ! command -v dbus-daemon &> /dev/null; then
    echo "错误: 未找到dbus-daemon，请安装dbus"
    exit 1
fi

# 启动服务器
echo "启动WEAK_NET服务器..."
./server/bin/weaknet-dbus-server
EOF

    # 创建客户端测试脚本
    cat > test-client.sh << 'EOF'
#!/bin/bash
# WEAK_NET 客户端测试脚本

cd "$(dirname "$0")"

# 检查服务器是否运行
if ! pgrep -f weaknet-dbus-server > /dev/null; then
    echo "错误: 服务器未运行，请先启动服务器"
    echo "运行: ./start-server.sh"
    exit 1
fi

# 运行客户端测试
make test-client COMMAND="${1:-all}"
EOF

    chmod +x start-server.sh test-client.sh
    
    print_success "启动脚本创建完成"
}

# 显示使用说明
show_usage() {
    print_info "WEAK_NET 项目安装完成!"
    echo
    echo "使用方法:"
    echo "  启动服务器: ./start-server.sh"
    echo "  运行测试:   ./test-client.sh [command]"
    echo "  编译项目:   make all"
    echo "  清理项目:   make clean"
    echo
    echo "可用测试命令:"
    echo "  all          - 运行所有测试"
    echo "  get          - 获取网络接口信息"
    echo "  health       - 网络健康检查"
    echo "  events       - 事件监听测试"
    echo "  ping <host>  - Ping测试"
    echo
    echo "更多信息请查看 README.md"
}

# 主函数
main() {
    echo "WEAK_NET 项目安装脚本"
    echo "======================"
    
    # 检查参数
    if [ "$1" = "--install-deps" ]; then
        check_root
        detect_os
        install_dependencies
        exit 0
    fi
    
    # 检查依赖
    check_dependencies
    
    # 编译项目
    build_project
    
    # 运行测试
    run_tests
    
    # 创建启动脚本
    create_scripts
    
    # 显示使用说明
    show_usage
}

# 运行主函数
main "$@"
