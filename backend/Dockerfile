# YachiyoCPP Dockerfile

# 构建阶段
FROM ubuntu:22.04 AS builder

# 设置环境变量
ENV DEBIAN_FRONTEND=noninteractive
ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++

# 使用国内镜像源 (解决容器内无法访问 archive.ubuntu.com 的问题)
RUN sed -i 's|http://archive.ubuntu.com|http://mirrors.aliyun.com|g' /etc/apt/sources.list && \
    sed -i 's|http://security.ubuntu.com|http://mirrors.aliyun.com|g' /etc/apt/sources.list

# 安装构建依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    wget \
    curl \
    libssl-dev \
    libpq-dev \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    uuid-dev \
    pkg-config \
    libboost-all-dev \
    libspdlog-dev \
    libasio-dev \
    && rm -rf /var/lib/apt/lists/*

# 从源码安装 yaml-cpp (apt 版本 CMake target 名不兼容)
RUN git clone --depth 1 --branch 0.8.0 https://github.com/jbeder/yaml-cpp.git /tmp/yaml-cpp && \
    cd /tmp/yaml-cpp && mkdir build && cd build && \
    cmake .. -DYAML_BUILD_SHARED_LIBS=ON -DYAML_CPP_BUILD_TESTS=OFF && \
    make -j$(nproc) && make install && \
    rm -rf /tmp/yaml-cpp

# 从源码安装 Crow (apt 中无此包)
RUN git clone --depth 1 --branch v1.2.0 https://github.com/CrowCpp/Crow.git /tmp/crow && \
    cd /tmp/crow && mkdir build && cd build && \
    cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF && \
    make install && \
    rm -rf /tmp/crow

# 从源码安装 libpqxx (apt 版本过旧)
RUN git clone --depth 1 --branch 7.9.2 https://github.com/jtv/libpqxx.git /tmp/libpqxx && \
    cd /tmp/libpqxx && mkdir build && cd build && \
    cmake .. -DBUILD_SHARED_LIBS=ON -DSKIP_BUILD_TEST=ON && \
    make -j$(nproc) && make install && \
    rm -rf /tmp/libpqxx

# 从源码安装 hiredis (apt 版本无 CMake config)
RUN git clone --depth 1 --branch v1.2.0 https://github.com/redis/hiredis.git /tmp/hiredis && \
    cd /tmp/hiredis && mkdir build && cd build && \
    cmake .. -DBUILD_SHARED_LIBS=ON && \
    make -j$(nproc) && make install && \
    rm -rf /tmp/hiredis

# 刷新动态链接库缓存
RUN ldconfig

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

# 使用国内镜像源
RUN sed -i 's|http://archive.ubuntu.com|http://mirrors.aliyun.com|g' /etc/apt/sources.list && \
    sed -i 's|http://security.ubuntu.com|http://mirrors.aliyun.com|g' /etc/apt/sources.list

# 安装运行时依赖
RUN apt-get update && apt-get install -y \
    libssl3 \
    libpq5 \
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

# 从构建阶段复制源码编译的动态库 (hiredis, libpqxx)
COPY --from=builder /usr/local/lib/ /usr/local/lib/
RUN ldconfig

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