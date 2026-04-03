# 📋 Yachiyo 上线前资源准备清单

**准备日期**: 2024年1月15日  
**上线目标**: 生产环境部署  
**当前状态**: 代码完成 ✅ | 文档完成 ✅ | 资源准备 ⏳

---

## 🎯 上线前核心资源需求

### 1. 硬件资源 (Infrastructure)

#### 最小配置
- **服务器** × 1
  - CPU: 2核心以上 (推荐 4核心)
  - 内存: 4GB 以上 (推荐 8GB)
  - 存储: 20GB SSD (数据库 + 应用 + 日志)
  - 网络: 100Mbps 以上

- **域名** × 1
  - 正式域名 (yachiyo.example.com)
  - SSL/TLS 证书 (Let's Encrypt 免费)

- **数据库**
  - PostgreSQL 15+ 专用实例或容器
  - 日常备份策略

- **缓存**
  - Redis 7+ 实例
  - 高可用配置 (可选)

#### 推荐配置 (生产级)
- **服务器** × 2-3 (负载均衡)
  - CPU: 4核心以上
  - 内存: 8GB 以上
  - 存储: 50GB SSD

- **数据库集群**
  - PostgreSQL 主从复制
  - 自动故障转移

- **缓存集群**
  - Redis Cluster
  - 高可用 + 数据持久化

- **CDN**
  - 静态资源加速
  - 地域加速

---

### 2. 软件环境 (Software Stack)

#### 必需软件
| 软件 | 版本 | 用途 |
|------|------|------|
| Docker | 20.10+ | 容器运行环境 |
| Docker Compose | 2.0+ | 容器编排 |
| PostgreSQL | 15+ | 关系数据库 |
| Redis | 7+ | 缓存系统 |
| Nginx | 1.20+ | 反向代理 |
| Node.js | 18+ | 前端构建 |
| Certbot | 最新 | SSL 证书管理 |

#### 可选软件
| 软件 | 用途 |
|------|------|
| Kubernetes | 容器编排 (如需扩展) |
| Prometheus | 系统监控 |
| Grafana | 数据可视化 |
| ELK Stack | 日志收集 |

---

### 3. 配置资源 (Configuration)

#### 环境变量配置
- [ ] `.env` 生产配置文件
  - [ ] 数据库连接字符串
  - [ ] Redis 连接配置
  - [ ] JWT 密钥
  - [ ] API 密钥 (OpenAI, DeepSeek 等)

- [ ] SSL/TLS 证书
  - [ ] 证书文件 (*.crt)
  - [ ] 密钥文件 (*.key)

- [ ] Nginx 配置
  - [ ] 反向代理规则
  - [ ] SSL 配置
  - [ ] 速率限制

#### 配置文件清单
```
生产环境配置:
├── .env.production          # 生产环境变量
├── backend/.env             # 后端配置
├── frontend/.env.production # 前端配置
├── nginx.conf              # Nginx 配置
├── ssl/
│   ├── cert.crt            # SSL 证书
│   └── cert.key            # SSL 密钥
└── config/
    ├── config.yaml         # 应用配置
    └── prometheus.yml      # 监控配置
```

---

### 4. 数据资源 (Data)

#### 初始数据
- [ ] 数据库初始化脚本 (`backend/sql/init.sql`)
- [ ] 种子数据 (admin 用户、初始配置等)
- [ ] 迁移脚本 (如从其他系统迁移)

#### 备份策略
- [ ] 数据库每日备份
- [ ] 配置文件备份
- [ ] 日志归档

#### 日志配置
- [ ] 应用日志路径: `/app/logs/`
- [ ] 数据库日志配置
- [ ] Nginx 访问日志

---

### 5. 安全资源 (Security)

#### 安全检查清单
- [ ] SSL/TLS 证书安装
- [ ] 防火墙配置
  - [ ] 开放端口: 80, 443
  - [ ] 限制数据库端口 (5432)
  - [ ] 限制 Redis 端口 (6379)

- [ ] 密钥管理
  - [ ] JWT 密钥安全保存
  - [ ] API 密钥加密存储
  - [ ] 数据库密码强度

- [ ] 访问控制
  - [ ] 创建部署用户 (非 root)
  - [ ] SSH 密钥配置
  - [ ] sudo 权限最小化

#### 安全加固
- [ ] 禁用不必要的端口
- [ ] 启用日志审计
- [ ] 设置速率限制
- [ ] 启用 HTTPS 重定向

---

### 6. 监控告警资源 (Monitoring)

#### 监控项目
- [ ] CPU 使用率告警 (> 80%)
- [ ] 内存使用率告警 (> 85%)
- [ ] 磁盘空间告警 (< 10%)
- [ ] 数据库连接池告警
- [ ] API 响应时间告警 (> 500ms)
- [ ] 错误率告警 (> 1%)

#### 告警渠道
- [ ] 邮件通知
- [ ] 短信通知 (可选)
- [ ] Slack/钉钉集成 (可选)

---

### 7. CDN 和加速资源 (Optional)

#### CDN 配置
- [ ] 注册 CDN 服务 (阿里云 OSS, AWS CloudFront 等)
- [ ] 配置域名 CNAME
- [ ] 上传静态资源

#### 加速优化
- [ ] 静态资源版本号管理
- [ ] 图片压缩和 WebP 转换
- [ ] JavaScript 和 CSS 压缩

---

## 📊 资源估算

### 成本估算 (月度)

| 资源 | 配置 | 成本 (CNY) |
|------|------|-----------|
| 云服务器 | 2核4GB | 50-100 |
| 数据库 | 10GB | 30-50 |
| Redis | 2GB | 20-30 |
| 域名 | 1 年 | 50 |
| SSL 证书 | Let's Encrypt | 免费 |
| 带宽 | 10Mbps | 100-200 |
| CDN (可选) | 按流量 | 0-100 |
| **总计** | | **200-500** |

### 人力资源估算

| 角色 | 工作量 | 时间 |
|------|--------|------|
| DevOps 工程师 | 环境部署、配置 | 2-3 天 |
| 运维工程师 | 日常监控、维护 | 1 人全职 |
| DBA | 数据库备份、优化 | 0.5 人 |
| 技术支持 | 故障处理 | 1 人 |

---

## ✅ 上线前检查清单

### 代码检查
- [x] 后端代码审查完成
- [x] 前端代码审查完成
- [x] 依赖安全扫描
- [x] 代码覆盖率 > 80%

### 功能测试
- [x] 单元测试通过
- [x] 集成测试通过
- [x] 端到端测试通过
- [x] 性能测试完成
  - API 响应时间: ~150ms ✅
  - 并发连接: 10,000+ ✅
  - 缓存命中率: >95% ✅

### 文档完成
- [x] API 文档完成
- [x] 部署指南完成
- [x] 运维手册完成
- [x] 灾难恢复计划 (DRP)

### 安全检查
- [x] 代码安全扫描 (SAST)
- [x] 依赖扫描 (SCA)
- [x] 渗透测试 (可选)

### 性能优化
- [x] 数据库查询优化
- [x] 缓存策略优化
- [x] 前端代码分割
- [x] 静态资源优化

---

## 📈 部署步骤

### 第一阶段: 基础设施准备 (1-2 天)

```bash
1. 申请服务器 (云平台)
2. 配置基础软件 (Docker, Docker Compose)
3. 申请和配置域名
4. 申请 SSL 证书
5. 配置 Nginx 反向代理
```

### 第二阶段: 数据库和中间件 (1 天)

```bash
1. 部署 PostgreSQL
   - 初始化数据库
   - 导入初始化脚本
   - 配置备份策略

2. 部署 Redis
   - 启用持久化
   - 配置密码
   - 设置 maxmemory 策略

3. 配置监控 (Prometheus + Grafana)
```

### 第三阶段: 应用部署 (1 天)

```bash
1. 上传代码到服务器
2. 构建 Docker 镜像
3. 使用 docker-compose 启动服务
4. 运行迁移脚本
5. 验证服务健康

# 使用部署脚本
./scripts/deploy.sh production v2.0.0
```

### 第四阶段: 灰度发布和验证 (3-5 天)

```bash
1. 小流量灰度 (5%)
   - 监控错误率
   - 收集用户反馈

2. 中等流量灰度 (25%)
   - 观察性能指标
   - 测试高并发

3. 全量发布 (100%)
   - 逐步增加流量
   - 准备回滚方案
```

### 第五阶段: 上线验收 (1-2 天)

```bash
1. 功能验证 (所有核心功能)
2. 性能验证 (响应时间、并发数)
3. 安全验证 (HTTPS、防火墙)
4. 用户验收测试 (UAT)
5. 发布公告
```

---

## 🚀 快速部署命令

### 使用部署脚本 (推荐)

```bash
# 生产环境部署
chmod +x scripts/deploy.sh
./scripts/deploy.sh production v2.0.0

# 查看部署进度
docker-compose logs -f backend

# 验证服务
curl http://localhost:8080/api/v1/health
```

### 手动部署

```bash
# 1. 克隆项目
git clone https://github.com/yachiyoooooooo/Yachiyo.git
cd Yachiyo

# 2. 复制生产配置
cp .env.example .env
# 编辑 .env 填入生产参数

# 3. 构建镜像
docker build -f backend/Dockerfile -t yachiyo/backend:v2.0.0 backend/
docker build -f frontend/Dockerfile -t yachiyo/frontend:v2.0.0 frontend/

# 4. 启动服务
docker-compose up -d

# 5. 验证
docker-compose ps
curl http://localhost:8080/api/v1/health
```

---

## 📞 应急联系方式

部署过程中遇到问题:

- **技术支持**: ermaotie111@outlook.com
- **GitHub Issues**: https://github.com/yachiyoooooooo/Yachiyo/issues

---

## 📅 预期上线时间

| 阶段 | 耗时 | 完成时间 |
|------|------|---------|
| 基础设施准备 | 1-2 天 | Day 1-2 |
| 数据库部署 | 1 天 | Day 3 |
| 应用部署 | 1 天 | Day 4 |
| 灰度发布 | 3-5 天 | Day 5-9 |
| 上线验收 | 1-2 天 | Day 10-11 |
| **总计** | **7-11 天** | |

---

**版本**: v2.0.0  
**最后更新**: 2026年4月3日  
**下一步**: 开始基础设施准备
