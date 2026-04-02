# 📊 Yachiyo 系统监控与告警完整指南

**版本**: 1.0  
**更新时间**: 2026-04-02  
**适用范围**: Prometheus、Grafana、ELK Stack、告警规则

---

## 📑 目录

1. [监控架构](#监控架构)
2. [Prometheus 配置](#prometheus-配置)
3. [Grafana 仪表板](#grafana-仪表板)
4. [ELK Stack 日志](#elk-stack-日志)
5. [告警规则](#告警规则)
6. [故障排查指南](#故障排查指南)

---

## 监控架构

### 完整监控栈

```
┌─────────────────────────────────────────────────┐
│         应用层指标收集                          │
│  (Prometheus Client Library)                   │
├─────────────────────────────────────────────────┤
│                                                 │
│  HTTP 请求 → Prometheus /metrics 端点           │
│  Database 查询 → 自定义指标                      │
│  业务逻辑 → Event 发射                          │
│                                                 │
└────────────────────┬────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        ▼                         ▼
   ┌─────────────┐        ┌──────────────┐
   │ Prometheus  │        │ Application  │
   │  (时间序列) │        │  Logs        │
   └──────┬──────┘        └──────┬───────┘
          │                      │
          └──────────┬───────────┘
                     ▼
        ┌────────────────────────────┐
        │   ELK Stack               │
        │   - Elasticsearch (存储)  │
        │   - Logstash (处理)       │
        │   - Kibana (可视化)       │
        └────────────┬───────────────┘
                     │
        ┌────────────┴───────────┐
        ▼                        ▼
   ┌─────────────┐        ┌──────────────┐
   │  Grafana    │        │ AlertManager │
   │  仪表板     │        │ 告警分派     │
   └─────────────┘        └──────┬───────┘
                                 │
                     ┌───────────┼───────────┐
                     ▼           ▼           ▼
                   Email      Slack        PagerDuty
```

### 关键指标分类

| 类别 | 指标 | 阈值 | 优先级 |
|------|------|------|--------|
| **系统** | CPU 使用率 | > 85% | 🔴 高 |
| | 内存使用率 | > 90% | 🔴 高 |
| | 磁盘使用率 | > 85% | 🟠 中 |
| **应用** | 错误率 | > 1% | 🔴 高 |
| | P99 延迟 | > 500ms | 🟠 中 |
| | QPS | < 100 | 🟡 低 |
| **数据库** | 连接数 | > 80% | 🔴 高 |
| | 慢查询 | > 1s | 🟠 中 |
| | 死锁 | > 0 | 🔴 高 |
| **缓存** | 命中率 | < 70% | 🟠 中 |
| | 驱逐数 | > 100/min | 🟡 低 |

---

## Prometheus 配置

### prometheus.yml

```yaml
# monitoring/prometheus.yml

global:
  scrape_interval: 15s  # 默认抓取间隔
  evaluation_interval: 15s  # 规则评估间隔
  external_labels:
    cluster: 'yachiyo'
    environment: 'production'

# 告警配置
alerting:
  alertmanagers:
    - static_configs:
        - targets:
          - localhost:9093

# 规则文件
rule_files:
  - 'rules/*.yml'

scrape_configs:
  # Prometheus 自身指标
  - job_name: 'prometheus'
    static_configs:
      - targets: ['localhost:9090']

  # 后端应用
  - job_name: 'yachiyo-backend'
    metrics_path: '/metrics'
    scrape_interval: 10s
    scrape_timeout: 5s
    static_configs:
      - targets: ['localhost:8080']
    relabel_configs:
      - source_labels: [__address__]
        target_label: instance
      - source_labels: [__scheme__]
        target_label: scheme

  # Nginx
  - job_name: 'nginx'
    static_configs:
      - targets: ['localhost:9113']

  # PostgreSQL
  - job_name: 'postgres'
    static_configs:
      - targets: ['localhost:9187']

  # Redis
  - job_name: 'redis'
    static_configs:
      - targets: ['localhost:9121']

  # Node Exporter (系统指标)
  - job_name: 'node'
    scrape_interval: 30s
    static_configs:
      - targets: ['localhost:9100']
    relabel_configs:
      - source_labels: [__address__]
        target_label: instance

  # Docker
  - job_name: 'docker'
    static_configs:
      - targets: ['localhost:9323']
```

### 应用集成

```cpp
// 在 C++ 后端集成 Prometheus 指标

#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#include <prometheus/exposition_serializer.h>

class MetricsCollector {
private:
    std::shared_ptr<prometheus::Registry> registry;
    
    // HTTP 请求指标
    prometheus::Counter& http_requests_total;
    prometheus::Counter& http_request_errors_total;
    prometheus::Histogram& http_request_duration_seconds;
    
    // 数据库指标
    prometheus::Counter& db_queries_total;
    prometheus::Gauge& db_connections_active;
    prometheus::Histogram& db_query_duration_seconds;
    
    // 缓存指标
    prometheus::Counter& cache_hits_total;
    prometheus::Counter& cache_misses_total;
    prometheus::Gauge& cache_size_bytes;
    
    // 业务指标
    prometheus::Counter& messages_sent_total;
    prometheus::Counter& translations_completed_total;
    prometheus::Gauge& active_conversations;

public:
    MetricsCollector()
        : registry(std::make_shared<prometheus::Registry>()),
          http_requests_total(prometheus::BuildCounter()
            .Name("http_requests_total")
            .Help("Total HTTP requests")
            .Register(*registry)
            .Add({{}})),
          http_request_errors_total(prometheus::BuildCounter()
            .Name("http_request_errors_total")
            .Help("Total HTTP errors")
            .Register(*registry)
            .Add({{}})),
          http_request_duration_seconds(prometheus::BuildHistogram()
            .Name("http_request_duration_seconds")
            .Help("HTTP request duration in seconds")
            .Register(*registry)
            .Add({{}, {.001, .01, .1, 1, 10}})),
          // ... 其他指标初始化
          db_queries_total(prometheus::BuildCounter()
            .Name("db_queries_total")
            .Help("Total database queries")
            .Register(*registry)
            .Add({{}})),
          db_connections_active(prometheus::BuildGauge()
            .Name("db_connections_active")
            .Help("Active database connections")
            .Register(*registry)
            .Add({{}})),
          cache_hits_total(prometheus::BuildCounter()
            .Name("cache_hits_total")
            .Help("Total cache hits")
            .Register(*registry)
            .Add({{}})),
          cache_misses_total(prometheus::BuildCounter()
            .Name("cache_misses_total")
            .Help("Total cache misses")
            .Register(*registry)
            .Add({{}})),
          messages_sent_total(prometheus::BuildCounter()
            .Name("messages_sent_total")
            .Help("Total messages sent")
            .Register(*registry)
            .Add({{}}))
    {}
    
    // 记录 HTTP 请求
    void recordHttpRequest(const std::string& method, int status, double duration) {
        http_requests_total.Increment();
        http_request_duration_seconds.Observe(duration);
        
        if (status >= 400) {
            http_request_errors_total.Increment();
        }
    }
    
    // 记录数据库查询
    void recordDatabaseQuery(double duration) {
        db_queries_total.Increment();
        db_query_duration_seconds.Observe(duration);
    }
    
    // 更新活跃连接数
    void setActiveConnections(int count) {
        db_connections_active.Set(count);
    }
    
    // 记录缓存命中/未命中
    void recordCacheHit() {
        cache_hits_total.Increment();
    }
    
    void recordCacheMiss() {
        cache_misses_total.Increment();
    }
    
    // 获取注册表用于 /metrics 端点
    std::string serialize() {
        auto serializer = prometheus::TextSerializer{};
        return serializer.Serialize(registry->Collect());
    }
};

// 使用示例
class RequestHandler {
private:
    MetricsCollector& metrics;
    
public:
    crow::response handleRequest(const crow::request& req) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // 处理请求...
        auto response = processRequest(req);
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double>(end - start).count();
        
        // 记录指标
        metrics.recordHttpRequest(
            req.method_name,
            response.code,
            duration
        );
        
        return response;
    }
};
```

---

## Grafana 仪表板

### 仪表板 JSON 配置

```json
{
  "dashboard": {
    "title": "Yachiyo 系统概览",
    "description": "实时系统监控仪表板",
    "timezone": "browser",
    "schemaVersion": 30,
    "version": 1,
    "panels": [
      {
        "id": 1,
        "title": "系统 CPU 使用率",
        "targets": [
          {
            "expr": "100 - (avg(rate(node_cpu_seconds_total{mode=\"idle\"}[5m])) * 100)",
            "legendFormat": "CPU Usage %"
          }
        ],
        "type": "graph",
        "gridPos": {"x": 0, "y": 0, "w": 12, "h": 8}
      },
      {
        "id": 2,
        "title": "内存使用率",
        "targets": [
          {
            "expr": "(1 - (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes)) * 100",
            "legendFormat": "Memory Usage %"
          }
        ],
        "type": "gauge",
        "gridPos": {"x": 12, "y": 0, "w": 12, "h": 8}
      },
      {
        "id": 3,
        "title": "HTTP 请求速率",
        "targets": [
          {
            "expr": "rate(http_requests_total[1m])",
            "legendFormat": "{{method}} {{path}}"
          }
        ],
        "type": "graph",
        "gridPos": {"x": 0, "y": 8, "w": 12, "h": 8}
      },
      {
        "id": 4,
        "title": "错误率",
        "targets": [
          {
            "expr": "rate(http_request_errors_total[5m]) / rate(http_requests_total[5m]) * 100",
            "legendFormat": "Error Rate %"
          }
        ],
        "type": "stat",
        "gridPos": {"x": 12, "y": 8, "w": 12, "h": 8}
      },
      {
        "id": 5,
        "title": "P99 响应延迟",
        "targets": [
          {
            "expr": "histogram_quantile(0.99, rate(http_request_duration_seconds_bucket[5m]))",
            "legendFormat": "P99 {{path}}"
          }
        ],
        "type": "graph",
        "gridPos": {"x": 0, "y": 16, "w": 12, "h": 8}
      },
      {
        "id": 6,
        "title": "数据库连接数",
        "targets": [
          {
            "expr": "db_connections_active",
            "legendFormat": "Active Connections"
          }
        ],
        "type": "graph",
        "gridPos": {"x": 12, "y": 16, "w": 12, "h": 8}
      },
      {
        "id": 7,
        "title": "缓存命中率",
        "targets": [
          {
            "expr": "rate(cache_hits_total[5m]) / (rate(cache_hits_total[5m]) + rate(cache_misses_total[5m])) * 100",
            "legendFormat": "Cache Hit Rate %"
          }
        ],
        "type": "gauge",
        "gridPos": {"x": 0, "y": 24, "w": 12, "h": 8}
      }
    ]
  }
}
```

---

## ELK Stack 日志

### Logstash 配置

```conf
# monitoring/logstash.conf

input {
  file {
    path => "/var/log/yachiyo/app.log"
    start_position => "beginning"
    codec => "json"
  }
}

filter {
  # 解析 timestamp
  date {
    match => [ "timestamp", "ISO8601" ]
    target => "@timestamp"
  }
  
  # 添加服务名称
  mutate {
    add_field => { "service" => "yachiyo-backend" }
  }
  
  # 提取错误堆栈跟踪
  if [level] == "ERROR" {
    multiline {
      pattern => "^%{TIMESTAMP_ISO8601}"
      negate => true
      what => "previous"
    }
  }
}

output {
  elasticsearch {
    hosts => ["localhost:9200"]
    index => "yachiyo-%{+YYYY.MM.dd}"
  }
  
  # 同时输出到控制台用于调试
  stdout { codec => "rubydebug" }
}
```

### Kibana 仪表板

```json
{
  "version": "8.0.0",
  "objects": [
    {
      "type": "dashboard",
      "id": "yachiyo-logs-dashboard",
      "attributes": {
        "title": "Yachiyo 日志监控",
        "panels": [
          {
            "visualization": {
              "title": "错误日志分布",
              "type": "pie",
              "query": "level: ERROR"
            }
          },
          {
            "visualization": {
              "title": "响应时间趋势",
              "type": "line",
              "query": "response_time: *"
            }
          },
          {
            "visualization": {
              "title": "最常见的错误",
              "type": "table",
              "query": "level: ERROR",
              "aggregation": "error_type"
            }
          }
        ]
      }
    }
  ]
}
```

---

## 告警规则

### alert_rules.yml

```yaml
# monitoring/rules/alert_rules.yml

groups:
  - name: yachiyo_alerts
    interval: 15s
    
    rules:
      # 系统告警
      - alert: HighCPUUsage
        expr: '100 - (avg(rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100) > 85'
        for: 5m
        labels:
          severity: critical
          service: system
        annotations:
          summary: "高 CPU 使用率"
          description: "CPU 使用率超过 85%"
      
      - alert: HighMemoryUsage
        expr: '(1 - (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes)) * 100 > 90'
        for: 5m
        labels:
          severity: critical
          service: system
        annotations:
          summary: "高内存使用率"
          description: "内存使用率超过 90%"
      
      - alert: DiskSpaceLow
        expr: 'node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"} < 0.15'
        for: 5m
        labels:
          severity: warning
          service: system
        annotations:
          summary: "磁盘空间不足"
          description: "磁盘可用空间低于 15%"

      # 应用告警
      - alert: HighErrorRate
        expr: 'rate(http_request_errors_total[5m]) / rate(http_requests_total[5m]) > 0.01'
        for: 5m
        labels:
          severity: critical
          service: application
        annotations:
          summary: "错误率过高"
          description: "错误率超过 1%"
      
      - alert: HighResponseLatency
        expr: 'histogram_quantile(0.99, rate(http_request_duration_seconds_bucket[5m])) > 0.5'
        for: 5m
        labels:
          severity: warning
          service: application
        annotations:
          summary: "响应延迟过高"
          description: "P99 响应延迟超过 500ms"
      
      - alert: LowThroughput
        expr: 'rate(http_requests_total[5m]) < 100'
        for: 10m
        labels:
          severity: warning
          service: application
        annotations:
          summary: "吞吐量过低"
          description: "请求速率低于 100 req/s"

      # 数据库告警
      - alert: DatabaseConnectionPoolExhausted
        expr: 'db_connections_active / 50 > 0.9'
        for: 5m
        labels:
          severity: critical
          service: database
        annotations:
          summary: "数据库连接池将满"
          description: "活跃连接数接近最大值"
      
      - alert: DatabaseSlowQueries
        expr: 'rate(db_queries_slow_total[5m]) > 10'
        for: 5m
        labels:
          severity: warning
          service: database
        annotations:
          summary: "慢查询过多"
          description: "检测到超过 10 个/s 的慢查询"
      
      - alert: DatabaseDown
        expr: 'up{job="postgres"} == 0'
        for: 1m
        labels:
          severity: critical
          service: database
        annotations:
          summary: "数据库宕机"
          description: "PostgreSQL 无法连接"

      # 缓存告警
      - alert: LowCacheHitRate
        expr: 'rate(cache_hits_total[5m]) / (rate(cache_hits_total[5m]) + rate(cache_misses_total[5m])) < 0.7'
        for: 10m
        labels:
          severity: warning
          service: cache
        annotations:
          summary: "缓存命中率过低"
          description: "缓存命中率低于 70%"
      
      - alert: RedisDown
        expr: 'up{job="redis"} == 0'
        for: 1m
        labels:
          severity: critical
          service: cache
        annotations:
          summary: "Redis 宕机"
          description: "Redis 无法连接"

      # 业务告警
      - alert: NoMessagesProcessed
        expr: 'rate(messages_sent_total[5m]) == 0'
        for: 10m
        labels:
          severity: warning
          service: business
        annotations:
          summary: "消息处理停止"
          description: "没有处理消息"
      
      - alert: TranslationServiceDown
        expr: 'up{job="translator"} == 0'
        for: 5m
        labels:
          severity: critical
          service: translator
        annotations:
          summary: "翻译服务宕机"
          description: "翻译服务无法连接"
```

---

## 故障排查指南

### 常见问题诊断

```bash
#!/bin/bash
# troubleshoot.sh - 故障诊断工具

set -e

SERVICE="${1:-yachiyo-backend}"

echo "=== Yachiyo 故障诊断工具 ==="
echo "服务: $SERVICE"
echo ""

# 1. 检查服务状态
echo "【1】检查服务状态..."
systemctl status $SERVICE || echo "⚠️ 服务未运行"

# 2. 检查端口
echo ""
echo "【2】检查网络端口..."
netstat -tuln | grep LISTEN || echo "⚠️ 未发现监听端口"

# 3. 检查日志
echo ""
echo "【3】检查最近的错误日志..."
journalctl -u $SERVICE -n 50 --no-pager || \
tail -50 /var/log/yachiyo/$SERVICE.log || \
echo "⚠️ 无法读取日志"

# 4. 检查系统资源
echo ""
echo "【4】系统资源使用..."
ps aux | grep $SERVICE | grep -v grep || echo "⚠️ 进程未找到"

# 5. 检查依赖服务
echo ""
echo "【5】检查依赖服务..."
echo "  - 数据库连接..."
pg_isready -h localhost -U yachiyo_user && echo "    ✅ PostgreSQL 正常" || echo "    ❌ PostgreSQL 无法连接"

echo "  - Redis 连接..."
redis-cli ping && echo "    ✅ Redis 正常" || echo "    ❌ Redis 无法连接"

# 6. 运行健康检查
echo ""
echo "【6】API 健康检查..."
curl -s http://localhost:8080/api/health | jq . && echo "✅ 健康检查通过" || echo "❌ 健康检查失败"

# 7. 性能指标
echo ""
echo "【7】性能指标..."
curl -s http://localhost:9090/api/v1/query?query='rate(http_requests_total[5m])' | jq '.data.result[].value[1]' || echo "⚠️ 无法获取指标"

echo ""
echo "=== 诊断完成 ==="
```

### 性能问题排查

```bash
#!/bin/bash
# diagnose_performance.sh - 性能问题诊断

echo "=== 性能问题诊断 ==="

# 1. 检查 CPU 使用率
echo "CPU 信息:"
top -bn1 | grep "Cpu(s)" | sed "s/.*, *\([0-9.]*\)%* id.*/\1/" | awk '{print "使用率: " (100 - $1) "%"}'

# 2. 检查内存
echo ""
echo "内存信息:"
free -m | awk 'NR==2{print "使用: " $3 "MB / 总计: " $2 "MB (" int($3/$2 * 100) "%)"}'

# 3. 检查 I/O
echo ""
echo "磁盘 I/O:"
iostat -x 1 2 | tail -10

# 4. 检查网络
echo ""
echo "网络连接:"
netstat -an | grep ESTABLISHED | wc -l | awk '{print "已建立连接: " $1}'

# 5. 检查数据库慢查询
echo ""
echo "数据库慢查询:"
psql -U yachiyo_user -d yachiyo_db -c \
  "SELECT query, mean_exec_time FROM pg_stat_statements WHERE mean_exec_time > 1000 ORDER BY mean_exec_time DESC LIMIT 10;"

# 6. 检查缓存命中率
echo ""
echo "缓存状态:"
redis-cli INFO stats | grep -E "hits|misses"
```

### 日志分析

```bash
#!/bin/bash
# analyze_logs.sh - 日志分析

LOG_FILE="${1:-/var/log/yachiyo/app.log}"

echo "=== 日志分析 ==="
echo "分析文件: $LOG_FILE"
echo ""

# 错误分布
echo "错误分布:"
grep "ERROR\|WARN" $LOG_FILE | awk '{print $NF}' | sort | uniq -c | sort -rn | head -20

# 响应时间统计
echo ""
echo "响应时间统计:"
grep "response_time" $LOG_FILE | awk -F'=' '{print $NF}' | awk '{sum+=$1; count++} END {print "平均: " sum/count "ms"}'

# 最常见的端点
echo ""
echo "最常见的端点:"
grep "GET\|POST\|PUT\|DELETE" $LOG_FILE | awk '{print $7}' | sort | uniq -c | sort -rn | head -10
```

---

**系统监控完成！** 📊

---

*最后更新: 2026-04-02*  
*下一步: 配置告警通知，建立值班制度*
