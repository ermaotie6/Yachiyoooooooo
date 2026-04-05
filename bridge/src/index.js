/**
 * Yachiyo ↔ OpenClaw 桥接服务 (精简版)
 * 
 * 唯一职责:
 *   接收 C++ 后端的 JSON → 转发给 OpenClaw → 把 OpenClaw 的 JSON 返回给 C++ 后端
 * 
 * 就这么简单。没有会话管理、没有异步回调、没有轮询。
 */

require('dotenv').config();
const express = require('express');
const axios = require('axios');
const logger = require('./logger');

// ==================== 配置 ====================

const CONFIG = {
    port: parseInt(process.env.BRIDGE_PORT || '8765'),
    openclawEndpoint: process.env.OPENCLAW_ENDPOINT || 'http://localhost:8000',
    openclawApiKey: process.env.OPENCLAW_API_KEY || '',
    requestTimeoutMs: parseInt(process.env.REQUEST_TIMEOUT_MS || '30000'),
};

// ==================== Express 服务 ====================

const app = express();
app.use(express.json({ limit: '10mb' }));

/**
 * 健康检查
 */
app.get('/health', (req, res) => {
    res.json({
        status: 'ok',
        service: 'yachiyo-openclaw-bridge',
        uptime: process.uptime(),
        openclawEndpoint: CONFIG.openclawEndpoint,
    });
});

/**
 * 核心端点: 接收 JSON → 转发 OpenClaw → 返回 JSON
 * 
 * C++ 后端 POST 到这里，桥接服务原样转发给 OpenClaw，
 * 拿到 OpenClaw 的响应后组装标准格式返回。
 * 
 * 请求体 (C++ 后端发来):
 * {
 *   "request_id": "uuid",
 *   "user_id": "user123",
 *   "text": "用户输入的文本",
 *   "context": "对话上下文",
 *   "emotion_hints": ["开心"],
 *   "max_tokens": 1000,
 *   "temperature": 0.7
 * }
 * 
 * 响应体 (返回给 C++ 后端):
 * {
 *   "request_id": "uuid",
 *   "success": true,
 *   "text": "AI 回复",
 *   "emotions": ["happy"],
 *   "actions": ["wave"],
 *   "processing_time_ms": 1200
 * }
 */
app.post('/process', async (req, res) => {
    const startTime = Date.now();
    const requestId = req.body.request_id || `req_${Date.now()}`;

    logger.info(`[${requestId}] 收到请求: user=${req.body.user_id}, text="${req.body.text?.substring(0, 50)}..."`);

    if (!req.body.text) {
        return res.status(400).json({
            request_id: requestId,
            success: false,
            error: '缺少 text 字段',
        });
    }

    try {
        // 直接转发给 OpenClaw
        const openclawResponse = await axios.post(
            `${CONFIG.openclawEndpoint}/process`,
            req.body,
            {
                timeout: CONFIG.requestTimeoutMs,
                headers: {
                    'Content-Type': 'application/json',
                    ...(CONFIG.openclawApiKey ? { 'Authorization': `Bearer ${CONFIG.openclawApiKey}` } : {}),
                },
            }
        );

        const result = openclawResponse.data;
        const processingTimeMs = Date.now() - startTime;

        // 组装标准响应返回给 C++ 后端
        const response = {
            request_id: requestId,
            success: true,
            text: result.text || '',
            emotions: result.emotions || [],
            actions: result.actions || [],
            processing_time_ms: processingTimeMs,
        };

        logger.info(`[${requestId}] 完成 (${processingTimeMs}ms): "${result.text?.substring(0, 80)}..."`);
        res.json(response);

    } catch (error) {
        const processingTimeMs = Date.now() - startTime;

        if (error.response) {
            logger.error(`[${requestId}] OpenClaw 错误: ${error.response.status}`);
            res.status(502).json({
                request_id: requestId,
                success: false,
                error: `OpenClaw 错误: ${error.response.status}`,
                detail: error.response.data,
                processing_time_ms: processingTimeMs,
            });
        } else if (error.code === 'ECONNREFUSED' || error.code === 'ETIMEDOUT') {
            logger.error(`[${requestId}] OpenClaw 不可达: ${error.message}`);
            res.status(503).json({
                request_id: requestId,
                success: false,
                error: 'OpenClaw 服务不可达',
                detail: error.message,
                processing_time_ms: processingTimeMs,
            });
        } else {
            logger.error(`[${requestId}] 未知错误: ${error.message}`);
            res.status(500).json({
                request_id: requestId,
                success: false,
                error: error.message,
                processing_time_ms: processingTimeMs,
            });
        }
    }
});

// ==================== 启动 ====================

app.listen(CONFIG.port, () => {
    logger.info(`📡 桥接服务已启动: http://0.0.0.0:${CONFIG.port}`);
    logger.info(`   POST /process  — C++ 后端发 JSON 到这里`);
    logger.info(`   GET  /health   — 健康检查`);
    logger.info(`🔗 OpenClaw: ${CONFIG.openclawEndpoint}`);
});

// 优雅退出
process.on('SIGTERM', () => {
    logger.info('收到 SIGTERM，正在关闭...');
    process.exit(0);
});

process.on('SIGINT', () => {
    logger.info('收到 SIGINT，正在关闭...');
    process.exit(0);
});
