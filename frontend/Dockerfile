# 前端 Yachiyo Vue 应用 Dockerfile

# 构建阶段
FROM node:18-alpine AS builder

# 设置工作目录
WORKDIR /app

# 复制 package.json 和 package-lock.json
COPY package*.json ./

# 安装依赖：如果存在 package-lock.json 使用 npm ci，否则回退到 npm install
RUN if [ -f package-lock.json ]; then \
            npm ci --prefer-offline --no-audit; \
        else \
            npm install --no-audit; \
        fi

# 复制源代码
COPY . .

# 构建应用
RUN npm run build

# 运行阶段
FROM node:18-alpine AS runtime

# 安装 serve 用于提供静态文件
RUN npm install -g serve

# 设置工作目录
WORKDIR /app

# 从构建阶段复制构建输出
COPY --from=builder /app/dist ./dist

# 创建非root用户
RUN addgroup -g 1001 -S yachiyo && adduser -S yachiyo -u 1001 -G yachiyo

# 设置用户
USER yachiyo

# 暴露端口
EXPOSE 3000

# 健康检查
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD node -e "require('http').get('http://localhost:3000', (r) => {if (r.statusCode !== 200) throw new Error(r.statusCode)})"

# 启动应用
CMD ["serve", "-s", "dist", "-l", "3000"]
