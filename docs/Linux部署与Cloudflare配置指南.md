# Linux 虚拟机部署 + Cloudflare 网络配置指南

> 适用于 Yachiyo 虚拟直播平台  
> 目标: 在 Linux VM 上部署完整项目，使用 qzz.io 免费域名 + Cloudflare DNS + HTTPS

---

## 1. 服务器环境准备

### 1.1 Arch Linux 安装基础包

```bash
# 更新系统
sudo pacman -Syu

# 安装基础开发工具
sudo pacman -S base-devel git wget curl

# 安装 Docker + Docker Compose
sudo pacman -S docker docker-compose
sudo systemctl enable --now docker
sudo usermod -aG docker $USER
# 重新登录使 docker 组生效

# 安装 Nginx
sudo pacman -S nginx
sudo systemctl enable nginx

# 安装 PostgreSQL（如果不用 Docker 内的）
sudo pacman -S postgresql
# 或者直接用 Docker 里的 PostgreSQL（推荐）

# 安装 Redis（如果不用 Docker 内的）
sudo pacman -S redis
# 同样推荐 Docker

# 安装 Node.js（构建前端）
sudo pacman -S nodejs npm

# 安装 CMake + C++ 编译器（构建后端）
sudo pacman -S cmake gcc

# 安装后端依赖库
sudo pacman -S libcurl-compat openssl libpqxx hiredis nlohmann-json

# 安装 Python（GPT-SoVITS / OpenClaw 需要）
sudo pacman -S python python-pip
```

### 1.2 Ubuntu/Debian 等效命令

```bash
sudo apt update && sudo apt upgrade -y
sudo apt install -y build-essential git wget curl cmake gcc g++ \
  libcurl4-openssl-dev libssl-dev libpqxx-dev libhiredis-dev \
  nlohmann-json3-dev docker.io docker-compose nginx \
  nodejs npm python3 python3-pip python3-venv

sudo systemctl enable --now docker
sudo usermod -aG docker $USER
```

---

## 2. 项目部署

### 2.1 克隆代码

```bash
cd /opt
sudo mkdir yachiyo && sudo chown $USER:$USER yachiyo
cd yachiyo
git clone https://github.com/ermaotie6/yachiyoooooooo.git .
```

### 2.2 构建后端

```bash
cd /opt/yachiyo/backend

# 创建构建目录
mkdir -p build && cd build

# CMake 配置
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译（使用所有核心）
make -j$(nproc)

# 编译产物在 build/src/yachiyo_server (或类似名称)
```

### 2.3 构建前端

```bash
cd /opt/yachiyo/frontend

# 安装依赖
npm install

# 配置环境变量
cat > .env.production << 'EOF'
VITE_API_BASE_URL=https://你的域名.qzz.io/api
VITE_WS_BASE_URL=wss://你的域名.qzz.io/ws
EOF

# 构建
npm run build

# 产物在 dist/ 目录
```

### 2.4 配置文件

```bash
cd /opt/yachiyo

# 编辑后端配置
nano backend/config/config.yaml
# 填入:
# - ai.deepseek.api_key
# - translation.engines[baidu].app_id / api_key
# - deepseek_moderation.api_key
# - jwt.secret_key（生成一个随机字符串）

# 生成 JWT 密钥
openssl rand -hex 32
```

### 2.5 Docker Compose 启动基础设施

```bash
cd /opt/yachiyo

# 创建 .env 文件
cat > .env << 'EOF'
DB_PASSWORD=your_strong_password_here
JWT_SECRET_KEY=your_jwt_secret_here
DEEPSEEK_API_KEY=your_deepseek_key_here
BAIDU_TRANSLATE_APP_ID=your_baidu_app_id
BAIDU_TRANSLATE_API_KEY=your_baidu_api_key
SOVITS_API_KEY=
EOF

# 启动 PostgreSQL + Redis
docker-compose up -d postgres redis

# 等待数据库就绪
sleep 5

# 初始化数据库
docker exec -i yachiyo-postgres psql -U yachiyo_app -d yachiyo < database/init.sql
```

---

## 3. Cloudflare + qzz.io 域名配置

### 3.1 获取 qzz.io 免费域名

1. 前往 qzz.io 注册并获取免费域名（例如 `yachiyo.qzz.io`）
2. 获取域名后，将 Nameserver 改为 Cloudflare 的 NS

### 3.2 Cloudflare 配置

1. 登录 [Cloudflare Dashboard](https://dash.cloudflare.com)
2. 添加站点 → 输入你的 qzz.io 域名
3. 选择免费计划

### 3.3 DNS 记录

在 Cloudflare DNS 设置中添加：

| 类型 | 名称 | 内容 | 代理 | TTL |
| ---- | ---- | ---- | ---- | ---- |
| A | `@` 或 `yachiyo` | `你的服务器公网IP` | ✅ 代理 | 自动 |
| AAAA | `@` 或 `yachiyo` | `你的服务器IPv6`（如有） | ✅ 代理 | 自动 |

> ⚠️ **重要**: 开启 Cloudflare 代理（橙色云朵图标），这样：
> - 隐藏真实服务器 IP
> - 自动获得 HTTPS（Cloudflare 边缘证书）
> - 获得 DDoS 防护和 CDN 加速

### 3.4 SSL/TLS 设置

在 Cloudflare → SSL/TLS 页面：

1. **加密模式**: 选择 **Full (Strict)**
2. 需要在服务器上安装 Cloudflare Origin Certificate:

```bash
# 在 Cloudflare → SSL/TLS → Origin Server 页面
# 点击 "Create Certificate"
# 选择 RSA (2048) 或 ECC
# Hostnames: *.qzz.io, qzz.io（或你的具体子域名）
# 有效期: 15 年

# 下载 Origin Certificate 和 Private Key
# 保存到服务器:
sudo mkdir -p /etc/nginx/ssl
sudo nano /etc/nginx/ssl/cloudflare-origin.pem   # 粘贴 Origin Certificate
sudo nano /etc/nginx/ssl/cloudflare-origin.key    # 粘贴 Private Key
sudo chmod 600 /etc/nginx/ssl/cloudflare-origin.key
```

### 3.5 Cloudflare 页面规则（可选）

- 缓存规则: 对 `/assets/*` 启用缓存
- 安全规则: 对 `/api/*` 限制请求频率

---

## 4. Nginx 配置

### 4.1 主配置文件

```bash
sudo nano /etc/nginx/nginx.conf
```

```nginx
user http;
worker_processes auto;
error_log /var/log/nginx/error.log warn;
pid /run/nginx.pid;

events {
    worker_connections 1024;
}

http {
    include       /etc/nginx/mime.types;
    default_type  application/octet-stream;

    sendfile on;
    keepalive_timeout 65;
    client_max_body_size 10m;

    # 日志格式
    log_format main '$remote_addr - $remote_user [$time_local] "$request" '
                    '$status $body_bytes_sent "$http_referer" '
                    '"$http_user_agent"';
    access_log /var/log/nginx/access.log main;

    # Gzip 压缩
    gzip on;
    gzip_types text/plain text/css application/json application/javascript text/xml;
    gzip_min_length 1000;

    # 速率限制
    limit_req_zone $binary_remote_addr zone=api:10m rate=30r/s;
    limit_req_zone $binary_remote_addr zone=ws:10m rate=5r/s;

    # WebSocket 升级映射
    map $http_upgrade $connection_upgrade {
        default upgrade;
        '' close;
    }

    # 上游服务
    upstream backend {
        server 127.0.0.1:8080;
    }

    upstream websocket {
        server 127.0.0.1:9001;
    }

    # HTTP → HTTPS 重定向（Cloudflare 代理时可省略，CF 自动处理）
    server {
        listen 80;
        server_name 你的域名.qzz.io;
        return 301 https://$host$request_uri;
    }

    # HTTPS 主服务
    server {
        listen 443 ssl http2;
        server_name 你的域名.qzz.io;

        # Cloudflare Origin Certificate
        ssl_certificate     /etc/nginx/ssl/cloudflare-origin.pem;
        ssl_certificate_key /etc/nginx/ssl/cloudflare-origin.key;
        ssl_protocols TLSv1.2 TLSv1.3;
        ssl_ciphers HIGH:!aNULL:!MD5;

        # 仅允许 Cloudflare IP 访问（安全加固）
        # Cloudflare IPv4 ranges: https://www.cloudflare.com/ips-v4
        # 取消注释以启用（定期更新 IP 列表）
        # allow 173.245.48.0/20;
        # allow 103.21.244.0/22;
        # allow 103.22.200.0/22;
        # allow 103.31.4.0/22;
        # allow 141.101.64.0/18;
        # allow 108.162.192.0/18;
        # allow 190.93.240.0/20;
        # allow 188.114.96.0/20;
        # allow 197.234.240.0/22;
        # allow 198.41.128.0/17;
        # allow 162.158.0.0/15;
        # allow 104.16.0.0/13;
        # allow 104.24.0.0/14;
        # allow 172.64.0.0/13;
        # allow 131.0.72.0/22;
        # deny all;

        # 前端静态文件
        root /opt/yachiyo/frontend/dist;
        index index.html;

        # SPA 路由回退
        location / {
            try_files $uri $uri/ /index.html;
        }

        # 静态资源缓存
        location /assets/ {
            expires 30d;
            add_header Cache-Control "public, immutable";
        }

        # API 代理
        location /api/ {
            limit_req zone=api burst=20 nodelay;

            proxy_pass http://backend;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $http_cf_connecting_ip;
            proxy_set_header X-Forwarded-For $http_x_forwarded_for;
            proxy_set_header X-Forwarded-Proto $scheme;

            # CORS（如果前后端同域则不需要）
            # add_header Access-Control-Allow-Origin *;
        }

        # WebSocket 代理
        location /ws {
            limit_req zone=ws burst=5 nodelay;

            proxy_pass http://websocket;
            proxy_http_version 1.1;
            proxy_set_header Upgrade $http_upgrade;
            proxy_set_header Connection $connection_upgrade;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $http_cf_connecting_ip;
            proxy_read_timeout 86400s;
            proxy_send_timeout 86400s;
        }

        # 健康检查
        location /health {
            proxy_pass http://backend/api/health;
        }
    }
}
```

### 4.2 验证 & 启动

```bash
# 测试配置
sudo nginx -t

# 启动/重载
sudo systemctl restart nginx
```

---

## 5. Systemd 服务配置

### 5.1 后端服务

```bash
sudo nano /etc/systemd/system/yachiyo-backend.service
```

```ini
[Unit]
Description=Yachiyo Backend Server
After=network.target docker.service
Wants=docker.service

[Service]
Type=simple
User=yachiyo
WorkingDirectory=/opt/yachiyo/backend
ExecStart=/opt/yachiyo/backend/build/src/yachiyo_server
Restart=always
RestartSec=5
Environment=DB_PASSWORD=your_password
Environment=JWT_SECRET_KEY=your_jwt_secret
Environment=DEEPSEEK_API_KEY=your_key
Environment=BAIDU_TRANSLATE_APP_ID=your_app_id
Environment=BAIDU_TRANSLATE_API_KEY=your_api_key

# 安全加固
NoNewPrivileges=true
ProtectSystem=strict
ReadWritePaths=/opt/yachiyo/logs /tmp

[Install]
WantedBy=multi-user.target
```

### 5.2 启动服务

```bash
# 创建专用用户
sudo useradd -r -s /usr/sbin/nologin yachiyo
sudo chown -R yachiyo:yachiyo /opt/yachiyo

# 启动
sudo systemctl daemon-reload
sudo systemctl enable --now yachiyo-backend
sudo systemctl status yachiyo-backend

# 查看日志
journalctl -u yachiyo-backend -f
```

---

## 6. 防火墙配置

### 6.1 iptables（或 nftables）

```bash
# 只开放必要端口
# 80: HTTP（Cloudflare → Nginx）
# 443: HTTPS（Cloudflare → Nginx）
# 22: SSH（管理用，建议改端口）

# Arch Linux (iptables)
sudo iptables -A INPUT -p tcp --dport 80 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 443 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 22 -j ACCEPT
sudo iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
sudo iptables -A INPUT -i lo -j ACCEPT
sudo iptables -P INPUT DROP

# 保存规则
sudo iptables-save > /etc/iptables/iptables.rules
sudo systemctl enable iptables
```

### 6.2 使用 ufw（Ubuntu 更方便）

```bash
sudo ufw default deny incoming
sudo ufw default allow outgoing
sudo ufw allow 22/tcp
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw enable
```

> **注意**: 内部端口（8080 后端, 9001 WebSocket, 5432 PostgreSQL, 6379 Redis, 5000 GPT-SoVITS, 8000 OpenClaw）**不要**对外开放，它们只通过 Nginx 或 Docker 内部网络访问。

---

## 7. Cloudflare WebSocket 支持

Cloudflare 免费版 **支持** WebSocket 代理，但需要确保：

1. **Nginx 配置中的 WebSocket 升级头已正确设置**（上面的配置已包含）

2. **Cloudflare Dashboard → Network**:
   - WebSockets: ✅ 已启用（免费版默认开启）

3. **前端 WebSocket URL** 使用 `wss://` 协议:

```env
# frontend/.env.production
VITE_WS_BASE_URL=wss://你的域名.qzz.io/ws
```

4. **超时设置**: Cloudflare 免费版 WebSocket 空闲超时 100 秒，需要确保心跳间隔 < 100s:

```yaml
# config/config.yaml
websocket:
  heartbeat_interval_ms: 30000  # 30 秒，远小于 100 秒限制
```

---

## 8. 完整启动顺序

```bash
# 1. 启动基础设施
cd /opt/yachiyo
docker-compose up -d postgres redis

# 2. 启动后端
sudo systemctl start yachiyo-backend

# 3. 启动 OpenClaw（如果有）
cd /opt/yachiyo/openclaw_simple
source venv/bin/activate
uvicorn main:app --host 127.0.0.1 --port 8000 &

# 4. 启动 GPT-SoVITS（如果有 GPU）
cd /opt/GPT-SoVITS
conda activate sovits
python api.py -a 127.0.0.1 -p 5000 -s models/sovits.pth -g models/gpt.ckpt &

# 5. 启动 Nginx
sudo systemctl start nginx

# 6. 验证
curl https://你的域名.qzz.io/health
curl https://你的域名.qzz.io/api/health
```

---

## 9. 监控 & 维护

### 9.1 日志查看

```bash
# 后端日志
journalctl -u yachiyo-backend -f

# Nginx 日志
tail -f /var/log/nginx/access.log
tail -f /var/log/nginx/error.log

# Docker 容器日志
docker logs yachiyo-postgres -f
docker logs yachiyo-redis -f
```

### 9.2 Prometheus + Grafana（可选）

项目已有 `backend/config/prometheus.yml` 配置文件，启用：

```bash
# 在 docker-compose.yml 中已有 prometheus 和 grafana 服务
docker-compose up -d prometheus grafana

# Grafana: http://localhost:3001 (admin/admin)
# 添加 Prometheus 数据源: http://prometheus:9090
```

### 9.3 备份

```bash
# PostgreSQL 备份
docker exec yachiyo-postgres pg_dump -U yachiyo_app yachiyo > backup_$(date +%Y%m%d).sql

# 自动备份 cron
echo "0 3 * * * docker exec yachiyo-postgres pg_dump -U yachiyo_app yachiyo > /opt/yachiyo/backups/backup_\$(date +\%Y\%m\%d).sql" | crontab -
```

---

## 10. 常见问题

### Q: Cloudflare 报 522 错误（Connection timed out）

- 确保服务器防火墙开放了 80 和 443 端口
- 确保 Nginx 正在运行: `sudo systemctl status nginx`
- 确保后端服务正在运行: `sudo systemctl status yachiyo-backend`

### Q: WebSocket 连接失败

- 检查 Nginx WebSocket 配置中的 `proxy_pass` 地址和端口
- 确保 `Upgrade` 和 `Connection` 头正确传递
- Cloudflare 免费版支持 WebSocket，无需额外设置

### Q: 域名无法解析

- 确认 qzz.io 的 nameserver 已指向 Cloudflare
- 在 Cloudflare DNS 中确认 A 记录指向正确的服务器 IP
- DNS 传播可能需要几分钟到几小时

### Q: SSL 证书错误

- Cloudflare SSL 模式设为 "Full (Strict)" 时，需要在服务器安装 Origin Certificate
- 如果是自签证书，SSL 模式可设为 "Full"（不推荐生产环境）

### Q: 服务器性能不足

- 最低建议: 2 核 4GB RAM（不含 GPT-SoVITS）
- 包含 GPT-SoVITS: 4 核 8GB RAM + GPU
- 可以将 GPT-SoVITS 部署在独立的 GPU 服务器上，后端通过内网访问
