# YachiyoCPP Dockerfile

# 构建阶段
FROM ubuntu:22.04 AS builder

# 设置环境变量
ENV DEBIAN_FRONTEND=noninteractive
ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++

# 安装构建依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    wget \
    curl \
    libssl-dev \
    libyaml-cpp-dev \
    libpq-dev \
    libhiredis-dev \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# 创建工作目录
WORKDIR /app

# 复制源代码
COPY . .

# 创建构建目录
RUN mkdir -p build

# 配置项目
WORKDIR /app/build
RUN cmake .. \
    -G "Ninja" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=20 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_INSTALL_PREFIX=/usr/local

# 构建项目
RUN ninja

# 安装到系统目录
RUN ninja install

# 运行阶段
FROM ubuntu:22.04 AS runtime

# 设置环境变量
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

# 安装运行时依赖
RUN apt-get update && apt-get install -y \
    libssl3 \
    libyaml-cpp0.7 \
    libpq5 \
    libhiredis0.14 \
    libcurl4 \
    ca-certificates \
    tzdata \
    && rm -rf /var/lib/apt/lists/*

# 设置时区
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# 创建非root用户
RUN groupadd -r yachiyo && useradd -r -g yachiyo -s /bin/bash -m yachiyo

# 创建工作目录
WORKDIR /app

# 从构建阶段复制可执行文件
COPY --from=builder /usr/local/bin/yachiyo_cpp /usr/local/bin/yachiyo_cpp

# 复制配置文件
COPY --from=builder /app/config /app/config

# 创建日志目录
RUN mkdir -p /app/logs && chown -R yachiyo:yachiyo /app/logs

# 切换用户
USER yachiyo

# 暴露端口 (HTTP 8080, WebSocket 9001)
EXPOSE 8080 9001

# 健康检查
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/api/v1/health || exit 1

# 设置入口点
ENTRYPOINT ["/usr/local/bin/yachiyo_cpp"]

# 默认命令
CMD ["--config-dir", "/app/config", "--env", "production"]