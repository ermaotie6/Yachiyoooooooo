# 🐳 Yachiyo 容器化与 CI/CD 完整指南

**版本**: 1.0  
**更新时间**: 2026-04-02  
**适用范围**: Docker、Kubernetes、GitHub Actions、GitLab CI

---

## 📑 目录

1. [Docker 容器化](#docker-容器化)
2. [Docker Compose 本地开发](#docker-compose-本地开发)
3. [Kubernetes 部署](#kubernetes-部署)
4. [GitHub Actions CI/CD](#github-actions-cicd)
5. [GitLab CI/CD](#gitlab-cicd)
6. [蓝绿部署策略](#蓝绿部署策略)
7. [灾难恢复](#灾难恢复)

---

## Docker 容器化

### 后端 Dockerfile

```dockerfile
# Dockerfile.backend - 多阶段构建

# 阶段 1: 构建
FROM gcc:11 as builder

WORKDIR /app

# 安装依赖
RUN apt-get update && apt-get install -y \
    cmake \
    libpq-dev \
    redis-tools \
    git \
    && rm -rf /var/lib/apt/lists/*

# 复制源代码
COPY . .

# 编译
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . -- -j$(nproc)

# 阶段 2: 运行时
FROM ubuntu:20.04

WORKDIR /app

# 安装运行时依赖
RUN apt-get update && apt-get install -y \
    libpq5 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# 从构建阶段复制二进制文件
COPY --from=builder /app/build/bin/yachiyo_backend /app/yachiyo_backend
COPY config /app/config

# 创建非 root 用户
RUN useradd -m -s /bin/bash yachiyo && \
    chown -R yachiyo:yachiyo /app

USER yachiyo

EXPOSE 8080

# 健康检查
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/api/health || exit 1

CMD ["/app/yachiyo_backend"]
```

### 前端 Dockerfile

```dockerfile
# Dockerfile.frontend - Vue + Vite

# 阶段 1: 构建
FROM node:18 as builder

WORKDIR /app

# 复制依赖文件
COPY package*.json ./

# 安装依赖
RUN npm ci

# 复制源代码
COPY . .

# 构建
RUN npm run build

# 阶段 2: 运行时 (Nginx)
FROM nginx:1.24-alpine

# 复制 Nginx 配置
COPY nginx.conf /etc/nginx/nginx.conf
COPY default.conf /etc/nginx/conf.d/default.conf

# 从构建阶段复制构建产物
COPY --from=builder /app/dist /usr/share/nginx/html

# Nginx 日志
RUN ln -sf /dev/stdout /var/log/nginx/access.log && \
    ln -sf /dev/stderr /var/log/nginx/error.log

EXPOSE 80

HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD wget --quiet --tries=1 --spider http://localhost/health || exit 1

CMD ["nginx", "-g", "daemon off;"]
```

### Nginx 配置文件

```nginx
# nginx.conf - Yachiyo 前端服务器配置

user nginx;
worker_processes auto;
error_log /var/log/nginx/error.log warn;
pid /var/run/nginx.pid;

events {
    worker_connections 4096;
    use epoll;
}

http {
    include /etc/nginx/mime.types;
    default_type application/octet-stream;

    log_format main '$remote_addr - $remote_user [$time_local] "$request" '
                    '$status $body_bytes_sent "$http_referer" '
                    '"$http_user_agent" "$http_x_forwarded_for"';

    access_log /var/log/nginx/access.log main;

    # 性能优化
    sendfile on;
    tcp_nopush on;
    tcp_nodelay on;
    keepalive_timeout 65;
    types_hash_max_size 2048;
    
    # Gzip 压缩
    gzip on;
    gzip_vary on;
    gzip_proxied any;
    gzip_comp_level 6;
    gzip_types text/plain text/css text/xml text/javascript 
               application/json application/javascript application/xml+rss;

    # API 代理
    upstream backend {
        server yachiyo-backend:8080 max_fails=3 fail_timeout=30s;
    }

    server {
        listen 80;
        server_name _;

        # 根路径
        location / {
            root /usr/share/nginx/html;
            try_files $uri $uri/ /index.html;
            
            # 缓存设置
            expires 1h;
            add_header Cache-Control "public, max-age=3600";
        }

        # 静态资源长期缓存
        location ~* \.(js|css|png|jpg|jpeg|gif|ico|svg|woff|woff2|ttf|eot)$ {
            root /usr/share/nginx/html;
            expires 30d;
            add_header Cache-Control "public, max-age=2592000";
        }

        # API 代理
        location /api/ {
            proxy_pass http://backend;
            proxy_http_version 1.1;
            proxy_set_header Upgrade $http_upgrade;
            proxy_set_header Connection "upgrade";
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_set_header X-Forwarded-Proto $scheme;
            
            # 超时设置
            proxy_connect_timeout 60s;
            proxy_send_timeout 60s;
            proxy_read_timeout 60s;
        }

        # WebSocket 代理
        location /ws {
            proxy_pass http://backend;
            proxy_http_version 1.1;
            proxy_set_header Upgrade $http_upgrade;
            proxy_set_header Connection "upgrade";
            proxy_set_header Host $host;
            proxy_read_timeout 86400s;
        }

        # 健康检查
        location /health {
            access_log off;
            return 200 "ok\n";
            add_header Content-Type text/plain;
        }
    }
}
```

---

## Docker Compose 本地开发

### 完整 docker-compose.yml

```yaml
# docker-compose.yml - 完整开发环境

version: '3.8'

services:
  # PostgreSQL 数据库
  postgres:
    image: postgres:15-alpine
    container_name: yachiyo-postgres
    environment:
      POSTGRES_USER: yachiyo_user
      POSTGRES_PASSWORD: ${DB_PASSWORD:-secure_password}
      POSTGRES_DB: yachiyo_db
    volumes:
      - postgres_data:/var/lib/postgresql/data
      - ./scripts/init_db.sql:/docker-entrypoint-initdb.d/init.sql
    ports:
      - "5432:5432"
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U yachiyo_user -d yachiyo_db"]
      interval: 10s
      timeout: 5s
      retries: 5
    networks:
      - yachiyo-network

  # Redis 缓存
  redis:
    image: redis:7-alpine
    container_name: yachiyo-redis
    command: redis-server --appendonly yes --requirepass ${REDIS_PASSWORD:-redis_password}
    volumes:
      - redis_data:/data
    ports:
      - "6379:6379"
    healthcheck:
      test: ["CMD", "redis-cli", "-a", "${REDIS_PASSWORD:-redis_password}", "ping"]
      interval: 10s
      timeout: 5s
      retries: 5
    networks:
      - yachiyo-network

  # 后端服务
  backend:
    build:
      context: ./backend
      dockerfile: Dockerfile
    container_name: yachiyo-backend
    environment:
      DATABASE_URL: postgresql://yachiyo_user:${DB_PASSWORD:-secure_password}@postgres:5432/yachiyo_db
      REDIS_URL: redis://:${REDIS_PASSWORD:-redis_password}@redis:6379
      OPENAI_API_KEY: ${OPENAI_API_KEY}
      JWT_SECRET: ${JWT_SECRET:-your_jwt_secret_key}
      LOG_LEVEL: ${LOG_LEVEL:-info}
    depends_on:
      postgres:
        condition: service_healthy
      redis:
        condition: service_healthy
    ports:
      - "8080:8080"
    volumes:
      - ./backend:/app/backend
      - ./logs/backend:/app/logs
    networks:
      - yachiyo-network
    restart: unless-stopped

  # 前端服务（开发模式）
  frontend:
    build:
      context: ./frontend
      dockerfile: Dockerfile.dev
    container_name: yachiyo-frontend
    environment:
      VITE_API_BASE_URL: http://localhost:8080
    ports:
      - "5173:5173"
    volumes:
      - ./frontend:/app
      - /app/node_modules
    networks:
      - yachiyo-network
    command: npm run dev
    restart: unless-stopped

  # Nginx 反向代理
  nginx:
    image: nginx:1.24-alpine
    container_name: yachiyo-nginx
    ports:
      - "80:80"
      - "443:443"
    volumes:
      - ./nginx.conf:/etc/nginx/nginx.conf:ro
      - ./ssl:/etc/nginx/ssl:ro
      - ./logs/nginx:/var/log/nginx
    depends_on:
      - backend
      - frontend
    networks:
      - yachiyo-network
    restart: unless-stopped

  # 监控：Prometheus
  prometheus:
    image: prom/prometheus:latest
    container_name: yachiyo-prometheus
    ports:
      - "9090:9090"
    volumes:
      - ./monitoring/prometheus.yml:/etc/prometheus/prometheus.yml:ro
      - prometheus_data:/prometheus
    command:
      - '--config.file=/etc/prometheus/prometheus.yml'
      - '--storage.tsdb.path=/prometheus'
    networks:
      - yachiyo-network
    restart: unless-stopped

  # 监控：Grafana
  grafana:
    image: grafana/grafana:latest
    container_name: yachiyo-grafana
    environment:
      GF_SECURITY_ADMIN_PASSWORD: ${GRAFANA_PASSWORD:-admin}
      GF_USERS_ALLOW_SIGN_UP: "false"
    ports:
      - "3000:3000"
    volumes:
      - grafana_data:/var/lib/grafana
      - ./monitoring/grafana/provisioning:/etc/grafana/provisioning:ro
    depends_on:
      - prometheus
    networks:
      - yachiyo-network
    restart: unless-stopped

volumes:
  postgres_data:
  redis_data:
  prometheus_data:
  grafana_data:

networks:
  yachiyo-network:
    driver: bridge
```

### 环境变量文件

```bash
# .env.local - 本地开发

# 数据库
DB_PASSWORD=secure_password

# Redis
REDIS_PASSWORD=redis_password

# 后端
OPENAI_API_KEY=sk-xxxxxxxxxxxxx
JWT_SECRET=your_jwt_secret_key_change_me_in_production
LOG_LEVEL=debug

# Grafana
GRAFANA_PASSWORD=admin_password

# 前端
VITE_API_BASE_URL=http://localhost:8080
```

### 启动脚本

```bash
#!/bin/bash
# start_dev_environment.sh - 启动完整开发环境

set -e

echo "🚀 启动 Yachiyo 开发环境..."

# 加载环境变量
if [ ! -f .env.local ]; then
    echo "❌ 缺少 .env.local 文件"
    exit 1
fi

# 创建日志目录
mkdir -p logs/backend logs/nginx

# 启动所有服务
echo "📦 构建和启动 Docker Compose 服务..."
docker-compose -f docker-compose.yml up -d

# 等待服务启动
echo "⏳ 等待服务启动..."
sleep 10

# 检查服务健康状态
echo "🔍 检查服务健康状态..."

services=(
  "yachiyo-postgres:5432"
  "yachiyo-redis:6379"
  "yachiyo-backend:8080"
  "yachiyo-frontend:5173"
  "yachiyo-nginx:80"
)

for service in "${services[@]}"; do
  host=$(echo $service | cut -d: -f1)
  port=$(echo $service | cut -d: -f2)
  
  if nc -z $host $port 2>/dev/null; then
    echo "✅ $host:$port - 运行中"
  else
    echo "❌ $host:$port - 未响应"
  fi
done

echo ""
echo "✅ 开发环境启动完成！"
echo ""
echo "📍 访问地址:"
echo "  - 前端:      http://localhost"
echo "  - API:       http://localhost:8080/api"
echo "  - Grafana:   http://localhost:3000"
echo "  - Prometheus: http://localhost:9090"
echo ""
echo "💡 查看日志:"
echo "  docker-compose logs -f backend"
echo "  docker-compose logs -f frontend"
```

---

## Kubernetes 部署

### 命名空间和 ConfigMap

```yaml
# k8s/00-namespace.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: yachiyo
  labels:
    name: yachiyo

---
# k8s/01-configmap.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: yachiyo-config
  namespace: yachiyo
data:
  LOG_LEVEL: "info"
  VITE_API_BASE_URL: "https://api.yachiyo.example.com"
```

### 秘密管理

```yaml
# k8s/02-secret.yaml
apiVersion: v1
kind: Secret
metadata:
  name: yachiyo-secrets
  namespace: yachiyo
type: Opaque
stringData:
  DATABASE_URL: postgresql://user:password@postgres:5432/yachiyo_db
  REDIS_PASSWORD: redis_password
  OPENAI_API_KEY: sk-xxxxxxxxxxxxx
  JWT_SECRET: your_jwt_secret_key
  REGISTRY_AUTH: base64_encoded_docker_auth
```

### 后端部署

```yaml
# k8s/03-backend-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: yachiyo-backend
  namespace: yachiyo
  labels:
    app: yachiyo-backend
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  selector:
    matchLabels:
      app: yachiyo-backend
  template:
    metadata:
      labels:
        app: yachiyo-backend
    spec:
      serviceAccountName: yachiyo-backend
      
      # 亲和性规则：pod 分散在不同节点
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values:
                  - yachiyo-backend
              topologyKey: kubernetes.io/hostname
      
      containers:
      - name: yachiyo
        image: registry.example.com/yachiyo/backend:1.0
        imagePullPolicy: Always
        
        ports:
        - name: http
          containerPort: 8080
          protocol: TCP
        
        env:
        - name: DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: yachiyo-secrets
              key: DATABASE_URL
        - name: REDIS_PASSWORD
          valueFrom:
            secretKeyRef:
              name: yachiyo-secrets
              key: REDIS_PASSWORD
        - name: OPENAI_API_KEY
          valueFrom:
            secretKeyRef:
              name: yachiyo-secrets
              key: OPENAI_API_KEY
        - name: JWT_SECRET
          valueFrom:
            secretKeyRef:
              name: yachiyo-secrets
              key: JWT_SECRET
        - name: LOG_LEVEL
          valueFrom:
            configMapKeyRef:
              name: yachiyo-config
              key: LOG_LEVEL
        
        # 资源限制
        resources:
          requests:
            cpu: 250m
            memory: 256Mi
          limits:
            cpu: 500m
            memory: 512Mi
        
        # 存活探针
        livenessProbe:
          httpGet:
            path: /api/health
            port: http
          initialDelaySeconds: 30
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 3
        
        # 就绪探针
        readinessProbe:
          httpGet:
            path: /api/health
            port: http
          initialDelaySeconds: 10
          periodSeconds: 5
          timeoutSeconds: 3
          failureThreshold: 3
        
        # 启动探针（给应用足够时间启动）
        startupProbe:
          httpGet:
            path: /api/health
            port: http
          initialDelaySeconds: 0
          periodSeconds: 5
          timeoutSeconds: 3
          failureThreshold: 30

---
# k8s/04-backend-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: yachiyo-backend
  namespace: yachiyo
  labels:
    app: yachiyo-backend
spec:
  type: ClusterIP
  selector:
    app: yachiyo-backend
  ports:
  - name: http
    port: 8080
    targetPort: http
    protocol: TCP

---
# k8s/05-backend-hpa.yaml
apiVersion: autoscaling.k8s.io/v2
kind: HorizontalPodAutoscaler
metadata:
  name: yachiyo-backend-hpa
  namespace: yachiyo
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: yachiyo-backend
  minReplicas: 3
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 0
      policies:
      - type: Percent
        value: 100
        periodSeconds: 15
      - type: Pods
        value: 2
        periodSeconds: 15
      selectPolicy: Max
```

### 前端部署

```yaml
# k8s/06-frontend-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: yachiyo-frontend
  namespace: yachiyo
spec:
  replicas: 2
  selector:
    matchLabels:
      app: yachiyo-frontend
  template:
    metadata:
      labels:
        app: yachiyo-frontend
    spec:
      containers:
      - name: frontend
        image: registry.example.com/yachiyo/frontend:1.0
        ports:
        - containerPort: 80
        
        env:
        - name: VITE_API_BASE_URL
          valueFrom:
            configMapKeyRef:
              name: yachiyo-config
              key: VITE_API_BASE_URL
        
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
        
        livenessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 10
          periodSeconds: 10
        
        readinessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 5
          periodSeconds: 5

---
# k8s/07-frontend-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: yachiyo-frontend
  namespace: yachiyo
spec:
  type: ClusterIP
  selector:
    app: yachiyo-frontend
  ports:
  - port: 80
    targetPort: 80
```

### Ingress 配置

```yaml
# k8s/08-ingress.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: yachiyo-ingress
  namespace: yachiyo
  annotations:
    cert-manager.io/cluster-issuer: letsencrypt-prod
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/rate-limit: "100"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - yachiyo.example.com
    - api.yachiyo.example.com
    secretName: yachiyo-tls
  rules:
  - host: yachiyo.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: yachiyo-frontend
            port:
              number: 80
  - host: api.yachiyo.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: yachiyo-backend
            port:
              number: 8080
```

---

## GitHub Actions CI/CD

### 完整 CI/CD 工作流

```yaml
# .github/workflows/ci-cd.yml

name: CI/CD Pipeline

on:
  push:
    branches: [ main, develop ]
    paths:
      - 'backend/**'
      - 'frontend/**'
      - '.github/workflows/**'
  pull_request:
    branches: [ main, develop ]

env:
  REGISTRY: ghcr.io
  IMAGE_NAME: ${{ github.repository }}

jobs:
  # 代码质量检查
  quality-check:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: 设置 C++
        uses: actions/setup-cpp@v1
      
      - name: 安装依赖
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake libpq-dev
      
      - name: 代码静态分析 (clang-tidy)
        run: |
          cd backend
          mkdir build && cd build
          cmake ..
          clang-tidy ../src/**/*.cpp -- -I../include
      
      - name: 前端代码检查 (ESLint)
        run: |
          cd frontend
          npm ci
          npm run lint

  # 后端构建和测试
  backend-build-test:
    runs-on: ubuntu-latest
    needs: quality-check
    services:
      postgres:
        image: postgres:15-alpine
        env:
          POSTGRES_PASSWORD: test_pass
          POSTGRES_DB: test_db
        options: >-
          --health-cmd pg_isready
          --health-interval 10s
          --health-timeout 5s
          --health-retries 5
        ports:
          - 5432:5432
    
    steps:
      - uses: actions/checkout@v3
      
      - name: 构建后端
        working-directory: ./backend
        run: |
          mkdir build && cd build
          cmake .. -DCMAKE_BUILD_TYPE=Release
          cmake --build . -- -j$(nproc)
      
      - name: 运行单元测试
        working-directory: ./backend/build
        run: ctest --output-on-failure
        env:
          DATABASE_URL: postgresql://postgres:test_pass@localhost:5432/test_db
      
      - name: 上传构建产物
        uses: actions/upload-artifact@v3
        with:
          name: backend-build
          path: backend/build/bin/

  # 前端构建和测试
  frontend-build-test:
    runs-on: ubuntu-latest
    needs: quality-check
    steps:
      - uses: actions/checkout@v3
      
      - name: 设置 Node.js
        uses: actions/setup-node@v3
        with:
          node-version: '18'
          cache: 'npm'
          cache-dependency-path: 'frontend/package-lock.json'
      
      - name: 安装依赖
        working-directory: ./frontend
        run: npm ci
      
      - name: 构建
        working-directory: ./frontend
        run: npm run build
      
      - name: 运行测试
        working-directory: ./frontend
        run: npm run test
      
      - name: 上传覆盖率
        uses: codecov/codecov-action@v3
        with:
          files: ./frontend/coverage/lcov.info

  # Docker 镜像构建和推送
  docker-build:
    runs-on: ubuntu-latest
    needs: [backend-build-test, frontend-build-test]
    permissions:
      contents: read
      packages: write
    
    steps:
      - uses: actions/checkout@v3
      
      - name: 设置 Docker Buildx
        uses: docker/setup-buildx-action@v2
      
      - name: 登录 Container Registry
        uses: docker/login-action@v2
        with:
          registry: ${{ env.REGISTRY }}
          username: ${{ github.actor }}
          password: ${{ secrets.GITHUB_TOKEN }}
      
      - name: 提取元数据
        id: meta
        uses: docker/metadata-action@v4
        with:
          images: ${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}
          tags: |
            type=ref,event=branch
            type=sha
            type=semver,pattern={{version}}
      
      - name: 构建并推送后端镜像
        uses: docker/build-push-action@v4
        with:
          context: ./backend
          file: ./backend/Dockerfile
          push: ${{ github.event_name != 'pull_request' }}
          tags: ${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}/backend:${{ steps.meta.outputs.version }}
          cache-from: type=registry,ref=${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}/backend:buildcache
          cache-to: type=registry,ref=${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}/backend:buildcache,mode=max
      
      - name: 构建并推送前端镜像
        uses: docker/build-push-action@v4
        with:
          context: ./frontend
          file: ./frontend/Dockerfile
          push: ${{ github.event_name != 'pull_request' }}
          tags: ${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}/frontend:${{ steps.meta.outputs.version }}
          cache-from: type=registry,ref=${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}/frontend:buildcache
          cache-to: type=registry,ref=${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}/frontend:buildcache,mode=max

  # 集成测试
  integration-test:
    runs-on: ubuntu-latest
    needs: docker-build
    services:
      postgres:
        image: postgres:15-alpine
        env:
          POSTGRES_PASSWORD: test_pass
          POSTGRES_DB: yachiyo_db
      redis:
        image: redis:7-alpine
    
    steps:
      - uses: actions/checkout@v3
      
      - name: 拉取镜像
        run: |
          docker pull ${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}/backend:latest
          docker pull ${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}/frontend:latest
      
      - name: 运行 Docker Compose
        run: |
          docker-compose -f docker-compose.yml up -d
          sleep 30
      
      - name: 运行集成测试
        run: |
          npm run test:integration
        env:
          API_URL: http://localhost:8080

  # 部署到测试环境
  deploy-staging:
    if: github.ref == 'refs/heads/develop'
    runs-on: ubuntu-latest
    needs: integration-test
    
    steps:
      - uses: actions/checkout@v3
      
      - name: 部署到测试环境
        run: |
          ./deploy.sh staging
        env:
          DEPLOY_KEY: ${{ secrets.DEPLOY_KEY }}
          DEPLOY_HOST: staging.yachiyo.example.com

  # 部署到生产环境
  deploy-production:
    if: github.ref == 'refs/heads/main'
    runs-on: ubuntu-latest
    needs: integration-test
    environment: production
    
    steps:
      - uses: actions/checkout@v3
      
      - name: 部署到生产环境（金丝雀）
        run: |
          ./deploy.sh production --canary
        env:
          DEPLOY_KEY: ${{ secrets.DEPLOY_KEY }}
          DEPLOY_HOST: api.yachiyo.example.com
```

---

## 蓝绿部署策略

### 蓝绿部署脚本

```bash
#!/bin/bash
# deploy_blue_green.sh - 蓝绿部署自动化

set -e

ENVIRONMENT=${1:-production}
NEW_VERSION=${2:-latest}

echo "🚀 开始蓝绿部署..."
echo "环境: $ENVIRONMENT"
echo "新版本: $NEW_VERSION"

# 1. 检查当前版本
CURRENT_DEPLOYMENT=$(kubectl get deployment -n yachiyo -o json | jq -r '.items[0].metadata.name')
echo "当前部署: $CURRENT_DEPLOYMENT"

# 2. 确定蓝绿标签
if [[ "$CURRENT_DEPLOYMENT" == *"-blue" ]]; then
    CURRENT_COLOR="blue"
    NEW_COLOR="green"
else
    CURRENT_COLOR="green"
    NEW_COLOR="blue"
fi

echo "当前颜色: $CURRENT_COLOR"
echo "新颜色: $NEW_COLOR"

# 3. 创建新部署
echo "📦 创建 $NEW_COLOR 部署..."
kubectl set image deployment/yachiyo-$NEW_COLOR \
  yachiyo=registry.example.com/yachiyo/backend:$NEW_VERSION \
  -n yachiyo

# 4. 等待新部署就绪
echo "⏳ 等待 $NEW_COLOR 部署就绪..."
kubectl rollout status deployment/yachiyo-$NEW_COLOR -n yachiyo --timeout=5m

# 5. 运行烟雾测试
echo "🧪 运行烟雾测试..."
if ! ./run_smoke_tests.sh "http://yachiyo-$NEW_COLOR:8080"; then
    echo "❌ 烟雾测试失败，回滚..."
    kubectl scale deployment yachiyo-$NEW_COLOR --replicas=0 -n yachiyo
    exit 1
fi

# 6. 切换流量到新部署
echo "🔄 切换流量到 $NEW_COLOR..."
kubectl patch service yachiyo-backend -n yachiyo \
  -p "{\"spec\":{\"selector\":{\"version\":\"$NEW_COLOR\"}}}"

# 7. 监控新部署
echo "📊 监控新部署..."
sleep 60

# 检查错误率
ERROR_RATE=$(kubectl exec -n yachiyo svc/prometheus -- \
  curl -s 'http://localhost:9090/api/v1/query?query=rate(http_requests_total{job="yachiyo-backend",status=~"5.."}[5m])' | \
  jq '.data.result[0].value[1]' -r)

if (( $(echo "$ERROR_RATE > 0.01" | bc -l) )); then
    echo "❌ 错误率过高，回滚..."
    kubectl patch service yachiyo-backend -n yachiyo \
      -p "{\"spec\":{\"selector\":{\"version\":\"$CURRENT_COLOR\"}}}"
    exit 1
fi

# 8. 缩放旧部署
echo "📉 缩放旧部署..."
kubectl scale deployment yachiyo-$CURRENT_COLOR --replicas=0 -n yachiyo

echo "✅ 蓝绿部署完成！"
```

---

## 灾难恢复

### 快速回滚脚本

```bash
#!/bin/bash
# rollback.sh - 快速回滚脚本

set -e

echo "⚠️ 开始回滚流程..."

# 1. 获取前一个工作的版本
PREVIOUS_VERSION=$(kubectl rollout history deployment/yachiyo-backend -n yachiyo | tail -2 | head -1 | awk '{print $1}')

echo "回滚到版本: $PREVIOUS_VERSION"

# 2. 执行回滚
kubectl rollout undo deployment/yachiyo-backend -n yachiyo --to-revision=$PREVIOUS_VERSION

# 3. 等待回滚完成
kubectl rollout status deployment/yachiyo-backend -n yachiyo --timeout=5m

# 4. 验证服务
echo "🔍 验证服务..."
kubectl get pods -n yachiyo -l app=yachiyo-backend

echo "✅ 回滚完成"
```

### 备份和恢复

```bash
#!/bin/bash
# backup_restore.sh - 数据库备份和恢复

# 备份
backup_database() {
    echo "备份数据库..."
    kubectl exec -n yachiyo yachiyo-postgres-0 -- \
      pg_dump -U yachiyo_user yachiyo_db > backup_$(date +%Y%m%d_%H%M%S).sql
}

# 恢复
restore_database() {
    BACKUP_FILE=$1
    echo "从 $BACKUP_FILE 恢复..."
    kubectl exec -n yachiyo -i yachiyo-postgres-0 -- \
      psql -U yachiyo_user yachiyo_db < $BACKUP_FILE
}
```

---

**容器化和 CI/CD 配置完成！** 🚀

---

*最后更新: 2026-04-02*  
*下一步: 配置监控告警，建立故障恢复流程*
