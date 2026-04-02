# 🚀 Yachiyo 项目性能优化与扩展性指南

**版本**: 1.0  
**更新时间**: 2026-04-02  
**适用场景**: 生产环境优化、大规模部署

---

## 📑 目录

1. [性能基准测试](#性能基准测试)
2. [缓存优化策略](#缓存优化策略)
3. [数据库优化](#数据库优化)
4. [API 性能优化](#api-性能优化)
5. [前端性能优化](#前端性能优化)
6. [可扩展性设计](#可扩展性设计)
7. [压力测试和容量规划](#压力测试和容量规划)

---

## 性能基准测试

### 测试环境配置

```
硬件:
  - CPU: 4 核
  - 内存: 8GB
  - 存储: SSD 100GB

软件:
  - OS: Ubuntu 20.04 LTS
  - Database: PostgreSQL 15
  - Cache: Redis 7
  - Runtime: C++20, Node 18
```

### 基准指标目标

| 指标 | 目标值 | 测试工具 |
|------|--------|---------|
| **吞吐量** | > 1000 req/sec | Apache Bench / wrk |
| **延迟 P50** | < 50ms | wrk / JMeter |
| **延迟 P95** | < 200ms | wrk / JMeter |
| **延迟 P99** | < 500ms | wrk / JMeter |
| **最大并发** | > 1000 | wrk / Locust |
| **错误率** | < 0.1% | 所有工具 |
| **缓存命中率** | > 80% | Redis Monitoring |

### 测试脚本

```bash
#!/bin/bash
# load_test.sh - 负载测试脚本

set -e

# 目标 URL
TARGET="https://yachiyo.example.com"

echo "=== Yachiyo 性能基准测试 ==="
echo "目标: $TARGET"
echo "时间: $(date)"

# 1. 预热测试（100 并发，30 秒）
echo -e "\n[1/4] 预热测试..."
ab -n 3000 -c 100 -t 30 "$TARGET/api/health" || true

# 2. 吞吐量测试（200 并发，60 秒）
echo -e "\n[2/4] 吞吐量测试..."
ab -n 60000 -c 200 -g results_throughput.tsv "$TARGET/api/messages/history" | tee results_throughput.txt

# 3. 延迟测试（使用 wrk）
echo -e "\n[3/4] 延迟测试（wrk）..."
wrk -t4 -c100 -d60s \
  -s lua/requests.lua \
  "$TARGET/" | tee results_latency.txt

# 4. 长连接测试（WebSocket）
echo -e "\n[4/4] WebSocket 连接测试..."
websocket-bench -c 100 -n 1000 "wss://yachiyo.example.com/ws" 2>&1 | tee results_websocket.txt

echo -e "\n=== 测试完成 ==="
echo "结果文件:"
echo "  - results_throughput.tsv"
echo "  - results_latency.txt"
echo "  - results_websocket.txt"
```

### 监控指令盘

```bash
#!/bin/bash
# monitor_performance.sh - 实时性能监控

while true; do
    clear
    
    echo "=== Yachiyo 性能监控 $(date '+%H:%M:%S') ==="
    echo ""
    
    # CPU 和内存
    echo "【系统资源】"
    ps aux | grep yachiyo | grep -v grep | awk '{
        print "CPU: " $3 "% | 内存: " $6/1024 "MB"
    }'
    
    # 网络连接数
    echo ""
    echo "【网络连接】"
    netstat -an | grep ESTABLISHED | wc -l | awk '{print "已建立连接: " $1}'
    
    # PostgreSQL 连接
    echo ""
    echo "【数据库】"
    psql -U yachiyo_user -d yachiyo_db -c \
        "SELECT count(*) as connections FROM pg_stat_activity;" 2>/dev/null
    
    # Redis 信息
    echo ""
    echo "【缓存（Redis）】"
    redis-cli -a password INFO stats 2>/dev/null | grep -E "total_commands_processed|total_connections_received|keyspace_hits|keyspace_misses"
    
    # API 响应时间（从日志获取）
    echo ""
    echo "【API 统计】"
    tail -100 /var/log/yachiyo/app.log | grep "response_time" | tail -10 | awk '{print $0}'
    
    sleep 5
done
```

---

## 缓存优化策略

### 多层缓存架构

```
┌─────────────────┐
│   浏览器缓存     │ (HTTP Cache Headers)
│   (1-7 天)      │
└────────┬────────┘
         │
┌────────▼────────┐
│   CDN 缓存       │ (CloudFlare / CloudFront)
│   (1-24 小时)   │
└────────┬────────┘
         │
┌────────▼────────┐
│   Redis 缓存     │ (2-24 小时)
│   (内存)        │
└────────┬────────┘
         │
┌────────▼────────┐
│   应用内存缓存   │ (几分钟)
│   (LRU)         │
└────────┬────────┘
         │
┌────────▼────────┐
│   数据库         │ (持久化)
└─────────────────┘
```

### HTTP 缓存头配置

```cpp
// 设置缓存头
void setCacheHeaders(crow::response& res, CachePolicy policy) {
    switch (policy) {
        case CachePolicy::PUBLIC_LONG:  // 1 天
            res.set_header("Cache-Control", "public, max-age=86400");
            break;
        
        case CachePolicy::PUBLIC_SHORT:  // 1 小时
            res.set_header("Cache-Control", "public, max-age=3600");
            break;
        
        case CachePolicy::PRIVATE:  // 个人缓存，5 分钟
            res.set_header("Cache-Control", "private, max-age=300");
            break;
        
        case CachePolicy::NO_CACHE:  // 实时数据
            res.set_header("Cache-Control", "no-cache, no-store, must-revalidate");
            break;
    }
}

// 使用示例
auto response = getAvatarStatus();
setCacheHeaders(res, CachePolicy::PRIVATE);
```

### Redis 缓存优化

```cpp
// 缓存预热
class CacheManager {
private:
    Redis redis;
    std::unordered_map<std::string, std::string> local_cache;
    
public:
    void warmupCache() {
        // 1. 预加载热点数据
        auto translations = db.query(
            "SELECT source, target FROM translations "
            "WHERE access_count > 100 "
            "ORDER BY access_count DESC LIMIT 5000"
        );
        
        for (const auto& [src, tgt] : translations) {
            redis.set("translation:" + src, tgt, 86400);  // 24h TTL
        }
        
        // 2. 预加载用户会话
        auto active_users = db.query(
            "SELECT id, last_activity FROM users "
            "WHERE last_activity > NOW() - INTERVAL 1 DAY"
        );
        
        for (const auto& [id, activity] : active_users) {
            redis.set("session:" + id, packUserData(id), 3600);  // 1h TTL
        }
    }
    
    // 两级缓存查询
    std::string get(const std::string& key) {
        // 第 1 级: 本地内存缓存
        auto it = local_cache.find(key);
        if (it != local_cache.end()) {
            return it->second;
        }
        
        // 第 2 级: Redis 缓存
        auto redis_val = redis.get(key);
        if (redis_val) {
            local_cache[key] = *redis_val;
            return *redis_val;
        }
        
        // 第 3 级: 数据库查询
        return db.query(key);  // Fallback to DB
    }
    
    // 缓存失效
    void invalidate(const std::string& pattern) {
        // 删除本地缓存
        for (auto it = local_cache.begin(); it != local_cache.end(); ) {
            if (matchPattern(it->first, pattern)) {
                it = local_cache.erase(it);
            } else {
                ++it;
            }
        }
        
        // 删除 Redis 缓存
        redis.delete(pattern + "*");
    }
};
```

### 缓存失效策略

```cpp
// 事件驱动的缓存失效
class EventPublisher {
private:
    std::vector<CacheInvalidationListener*> listeners;
    
public:
    void onMessageCreated(const Message& msg) {
        // 失效涉及的缓存
        cache_mgr.invalidate("user_messages:" + msg.user_id);
        cache_mgr.invalidate("conversation:" + msg.conversation_id);
        
        // 发布事件给订阅者
        publish("message_created", msg);
    }
    
    void onTranslationComplete(const Translation& trans) {
        // 缓存新翻译
        cache_mgr.set(
            "translation:" + trans.source,
            trans.target,
            86400  // 1 天 TTL
        );
    }
};
```

---

## 数据库优化

### 索引策略

```sql
-- 1. 主查询优化
CREATE INDEX idx_users_last_activity 
    ON users(last_activity DESC) WHERE active = true;

CREATE INDEX idx_messages_user_time 
    ON messages(user_id, created_at DESC);

CREATE INDEX idx_translations_source_hash 
    ON translations(md5(source));

-- 2. 复合索引
CREATE INDEX idx_msg_user_conversation 
    ON messages(user_id, conversation_id, created_at DESC);

-- 3. 部分索引（只索引活跃数据）
CREATE INDEX idx_active_messages 
    ON messages(user_id, created_at DESC) 
    WHERE is_deleted = false AND is_archived = false;

-- 4. BRIN 索引（大表时序数据）
CREATE INDEX idx_logs_timestamp_brin 
    ON logs USING brin (created_at);

-- 查看索引使用情况
SELECT schemaname, tablename, indexname, idx_scan
FROM pg_stat_user_indexes
ORDER BY idx_scan DESC;
```

### 查询优化

```sql
-- ❌ 优化前: N+1 查询
-- 循环查询每条消息的发送者信息，非常低效

-- ✅ 优化后: 使用 JOIN
SELECT 
    m.id, m.content, m.created_at,
    u.id, u.username, u.avatar_url
FROM messages m
INNER JOIN users u ON m.sender_id = u.id
WHERE m.conversation_id = $1
ORDER BY m.created_at DESC
LIMIT 50;

-- 使用 EXPLAIN ANALYZE 分析查询计划
EXPLAIN ANALYZE
SELECT * FROM messages 
WHERE user_id = $1 AND created_at > NOW() - INTERVAL 7 DAY
ORDER BY created_at DESC
LIMIT 50;

-- 结果应该显示 index scan，而不是 sequential scan
```

### 连接池配置

```cpp
class DatabasePool {
private:
    std::queue<std::unique_ptr<DatabaseConnection>> connections;
    std::mutex pool_mutex;
    int max_size = 50;
    
public:
    std::unique_ptr<DatabaseConnection> acquire() {
        std::lock_guard<std::mutex> lock(pool_mutex);
        
        if (connections.empty()) {
            if (connections.size() < max_size) {
                connections.push(createConnection());
            } else {
                // 等待连接可用
                throw std::runtime_error("连接池已满");
            }
        }
        
        auto conn = std::move(connections.front());
        connections.pop();
        return conn;
    }
    
    void release(std::unique_ptr<DatabaseConnection> conn) {
        std::lock_guard<std::mutex> lock(pool_mutex);
        
        if (conn && conn->isValid()) {
            connections.push(std::move(conn));
        }
    }
};

// 使用 RAII 模式
class PooledConnection {
    DatabasePool& pool;
    std::unique_ptr<DatabaseConnection> conn;
    
public:
    PooledConnection(DatabasePool& p) : pool(p) {
        conn = pool.acquire();
    }
    
    ~PooledConnection() {
        if (conn) {
            pool.release(std::move(conn));
        }
    }
    
    DatabaseConnection* operator->() { return conn.get(); }
};
```

---

## API 性能优化

### 请求响应压缩

```cpp
// 启用 Gzip 压缩
crow::SimpleApp app;

app.get("/api/messages").methods("GET"_method)(
    [](const crow::request& req) {
        auto messages = getMessages();
        std::string json_response = json(messages).dump();
        
        // 检查客户端是否支持 gzip
        if (req.get_header_value("Accept-Encoding").find("gzip") != std::string::npos) {
            auto compressed = gzipCompress(json_response);
            crow::response res(compressed);
            res.set_header("Content-Encoding", "gzip");
            res.set_header("Content-Type", "application/json");
            return res;
        }
        
        return crow::response{json_response};
    }
);
```

### 分页和流式响应

```cpp
// 分页处理
app.get("/api/messages").methods("GET"_method)(
    [](const crow::request& req) {
        int page = std::stoi(req.url_params.get("page") ?: "1");
        int page_size = std::stoi(req.url_params.get("page_size") ?: "50");
        
        // 限制最大页面大小
        page_size = std::min(page_size, 100);
        
        int offset = (page - 1) * page_size;
        auto messages = db.query(
            "SELECT * FROM messages ORDER BY created_at DESC LIMIT ? OFFSET ?",
            page_size, offset
        );
        
        auto total = db.count("SELECT COUNT(*) FROM messages");
        
        return crow::response{nlohmann::json{
            {"data", messages},
            {"page", page},
            {"page_size", page_size},
            {"total", total},
            {"total_pages", (total + page_size - 1) / page_size}
        }.dump()};
    }
);

// 流式响应（大量数据）
app.get("/api/export/messages").methods("GET"_method)(
    [](const crow::request& req) {
        crow::response res;
        res.set_header("Content-Type", "application/octet-stream");
        res.set_header("Content-Disposition", "attachment; filename=messages.jsonl");
        
        // 流式输出，避免内存溢出
        db.stream_query("SELECT * FROM messages", [&res](const auto& row) {
            res.body += row.to_json().dump() + "\n";
        });
        
        return res;
    }
);
```

### 异步处理和后台任务

```cpp
class TaskQueue {
private:
    std::queue<Task> tasks;
    std::vector<std::thread> workers;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool shutdown = false;
    
public:
    TaskQueue(int num_workers = 4) {
        for (int i = 0; i < num_workers; ++i) {
            workers.emplace_back([this] { processWorker(); });
        }
    }
    
    void enqueue(Task task) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            tasks.push(task);
        }
        cv.notify_one();
    }
    
private:
    void processWorker() {
        while (true) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                cv.wait(lock, [this] { return !tasks.empty() || shutdown; });
                
                if (shutdown && tasks.empty()) break;
                if (tasks.empty()) continue;
                
                task = tasks.front();
                tasks.pop();
            }
            
            try {
                task.execute();
            } catch (const std::exception& e) {
                logger.error("任务执行失败: {}", e.what());
            }
        }
    }
};

// 使用示例
TaskQueue task_queue(8);  // 8 个工作线程

app.post("/api/messages/send").methods("POST"_method)(
    [](const crow::request& req) {
        auto msg = parseMessage(req.body);
        
        // 立即返回响应
        task_queue.enqueue(Task([msg] {
            // 后台处理：翻译、语音合成、AI 响应等
            auto translation = translate(msg.text);
            auto audio = synthesizeVoice(translation);
            auto ai_response = callAI(msg);
            
            // 保存结果到数据库
            db.insert("messages", {msg, translation, audio, ai_response});
        }));
        
        return crow::response(200, R"({"status": "received"})");
    }
);
```

---

## 前端性能优化

### 代码分割和懒加载

```typescript
// vite.config.ts
import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

export default defineConfig({
  plugins: [vue()],
  build: {
    rollupOptions: {
      output: {
        manualChunks: {
          'vendor': ['vue', 'vue-router', 'pinia', 'axios'],
          'ui': ['element-plus', '@element-plus/icons-vue'],
          'live2d': ['live2d']
        }
      }
    },
    // 启用代码压缩
    minify: 'terser',
    terserOptions: {
      compress: {
        drop_console: true  // 删除 console.log
      }
    }
  }
})
```

### 路由懒加载

```typescript
// router/index.ts
import { createRouter, createWebHistory } from 'vue-router'

const routes = [
  {
    path: '/',
    component: () => import('@/views/Home.vue')
  },
  {
    path: '/chat',
    component: () => import('@/views/Chat.vue')  // 按需加载
  },
  {
    path: '/profile',
    component: () => import('@/views/Profile.vue')
  }
]

export default createRouter({
  history: createWebHistory(),
  routes
})
```

### 图片优化

```typescript
// 使用 WebP 格式和响应式图片
<template>
  <picture>
    <!-- WebP 格式（现代浏览器） -->
    <source srcset="/images/avatar.webp" type="image/webp" />
    <!-- 降级为 PNG -->
    <img src="/images/avatar.png" alt="虚拱头像" />
  </picture>
</template>

<script setup>
// 图片预加载
const preloadImages = () => {
  const images = ['/images/avatar.webp', '/images/avatar.png']
  images.forEach(src => {
    const img = new Image()
    img.src = src
  })
}

onMounted(() => preloadImages())
</script>
```

### 性能监控

```typescript
// utils/performance.ts
export class PerformanceMonitor {
  static markEvent(name: string) {
    performance.mark(name)
  }
  
  static measureEvent(name: string, startMark: string, endMark: string) {
    performance.measure(name, startMark, endMark)
    const measure = performance.getEntriesByName(name)[0]
    console.log(`${name}: ${measure.duration.toFixed(2)}ms`)
  }
  
  static getMetrics() {
    const navigation = performance.getEntriesByType('navigation')[0] as PerformanceNavigationTiming
    
    return {
      // DNS 查询时间
      dns: navigation.domainLookupEnd - navigation.domainLookupStart,
      // TCP 连接时间
      tcp: navigation.connectEnd - navigation.connectStart,
      // 首字节时间
      ttfb: navigation.responseStart - navigation.requestStart,
      // DOM 解析时间
      domParse: navigation.domInteractive - navigation.domLoading,
      // 加载完成时间
      loadComplete: navigation.loadEventEnd - navigation.loadEventStart,
      // 首屏绘制时间
      fp: performance.getEntriesByName('first-paint')[0]?.startTime || 0,
      // 首次内容绘制
      fcp: performance.getEntriesByName('first-contentful-paint')[0]?.startTime || 0
    }
  }
}

// 使用
PerformanceMonitor.markEvent('chat-load-start')
// ... 加载聊天组件 ...
PerformanceMonitor.markEvent('chat-load-end')
PerformanceMonitor.measureEvent('chat-load', 'chat-load-start', 'chat-load-end')

console.log(PerformanceMonitor.getMetrics())
```

---

## 可扩展性设计

### 微服务架构

```
┌─────────────────────────────────────┐
│        API Gateway (Nginx)          │
├─────────────────────────────────────┤
│                                     │
├──────────────┬──────────────┬───────┤
│              │              │       │
▼              ▼              ▼       ▼
┌────────┐  ┌────────┐   ┌────────┐ ┌────────┐
│Avatar  │  │Chat    │   │Auth    │ │Media   │
│Service │  │Service │   │Service │ │Service │
└────────┘  └────────┘   └────────┘ └────────┘
    │           │           │          │
    └─────┬─────┴─────┬─────┴──────┬───┘
          │           │           │
          ▼           ▼           ▼
      ┌───────────────────────────────┐
      │    Message Queue (RabbitMQ)   │
      └───────────────────────────────┘
              │
      ┌───────┼────────┬────────┐
      ▼       ▼        ▼        ▼
   ┌──────┬──────┬──────┬──────┐
   │ DB   │Redis │Cache │ Log  │
   └──────┴──────┴──────┴──────┘
```

### 服务间通信

```cpp
// 使用 gRPC 进行高效的服务间通信
// proto/avatar_service.proto
syntax = "proto3";

service AvatarService {
  rpc GenerateResponse (ChatRequest) returns (AvatarResponse);
  rpc TranslateText (TranslateRequest) returns (TranslateResponse);
  rpc SynthesizeVoice (VoiceRequest) returns (VoiceResponse);
}

message ChatRequest {
  string user_id = 1;
  string text = 2;
  string language = 3;
}

message AvatarResponse {
  string response_text = 1;
  string translated_text = 2;
  bytes audio_data = 3;
  string animation_data = 4;
}
```

### 水平扩展

```bash
# Docker Compose 多实例部署
version: '3.8'

services:
  yachiyo-1:
    image: yachiyo:latest
    ports:
      - "8081:8080"
    environment:
      - NODE_ID=1
      - REDIS_URL=redis://redis-cluster:6379
  
  yachiyo-2:
    image: yachiyo:latest
    ports:
      - "8082:8080"
    environment:
      - NODE_ID=2
      - REDIS_URL=redis://redis-cluster:6379
  
  yachiyo-3:
    image: yachiyo:latest
    ports:
      - "8083:8080"
    environment:
      - NODE_ID=3
      - REDIS_URL=redis://redis-cluster:6379

  nginx:
    image: nginx:latest
    ports:
      - "80:80"
      - "443:443"
    volumes:
      - ./nginx.conf:/etc/nginx/nginx.conf
```

### Kubernetes 部署示例

```yaml
# deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: yachiyo-avatar-service
spec:
  replicas: 3
  selector:
    matchLabels:
      app: yachiyo-avatar
  template:
    metadata:
      labels:
        app: yachiyo-avatar
    spec:
      containers:
      - name: yachiyo
        image: yachiyo:1.0
        ports:
        - containerPort: 8080
        env:
        - name: DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: yachiyo-secrets
              key: database-url
        resources:
          requests:
            memory: "256Mi"
            cpu: "250m"
          limits:
            memory: "512Mi"
            cpu: "500m"
        livenessProbe:
          httpGet:
            path: /api/health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
---
apiVersion: v1
kind: Service
metadata:
  name: yachiyo-service
spec:
  selector:
    app: yachiyo-avatar
  ports:
  - protocol: TCP
    port: 80
    targetPort: 8080
  type: LoadBalancer
---
apiVersion: autoscaling.k8s.io/v2
kind: HorizontalPodAutoscaler
metadata:
  name: yachiyo-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: yachiyo-avatar-service
  minReplicas: 2
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
```

---

## 压力测试和容量规划

### 压力测试脚本

```bash
#!/bin/bash
# stress_test.sh - 完整压力测试

set -e

TARGET="${1:-https://yachiyo.example.com}"
DURATION="${2:-60}"
CONCURRENCY="${3:-100}"

echo "=== Yachiyo 压力测试 ==="
echo "目标: $TARGET"
echo "持续时间: ${DURATION}s"
echo "并发数: $CONCURRENCY"
echo ""

# 使用 wrk 进行压力测试
wrk -t4 \
  -c$CONCURRENCY \
  -d${DURATION}s \
  --timeout=5s \
  --latency \
  -s stress_test.lua \
  "$TARGET" | tee stress_test_results.txt

# 检查结果
if grep -q "socket: connection refused" stress_test_results.txt; then
    echo "❌ 连接失败 - 服务可能宕机"
    exit 1
elif grep -q "Error: " stress_test_results.txt; then
    echo "⚠️ 发现错误 - 检查详细日志"
    exit 1
else
    echo "✅ 压力测试通过"
    exit 0
fi
```

### 容量规划公式

```
QPS 容量 = (吞吐量 / 平均响应时间) × (1 - 错误率)

示例:
  - 吞吐量: 1000 req/sec
  - 平均响应时间: 100ms
  - 错误率: 0.1%
  
  QPS = (1000 / 0.1) × (1 - 0.001) = 9,990 req/sec
```

### 容量规划表

| 场景 | 预期 QPS | 推荐配置 | 成本估算 |
|------|---------|---------|---------|
| **小规模** (100 活跃用户) | 100 | 1 服务器 (2C4GB) | $200/月 |
| **中规模** (1,000 活跃用户) | 1,000 | 3 服务器 (2C4GB) | $600/月 |
| **大规模** (10,000 活跃用户) | 10,000 | 10 服务器 + LB + CDN | $3,000/月 |
| **超大规模** (100,000+ 活跃用户) | 100,000 | K8s 集群 + 全球 CDN | $15,000+/月 |

---

**性能优化完成！** 🚀

---

*最后更新: 2026-04-02*  
*下一步: 执行基准测试，监控实际指标，按需优化*
