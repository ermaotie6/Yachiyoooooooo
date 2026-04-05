/**
 * Yachiyo ↔ OpenClaw 桥接服务
 * 
 * 架构:
 *   C++ 后端 --HTTP POST :8765--> 桥接服务 --session--> OpenClaw
 *   C++ 后端 <--HTTP POST :8766-- 桥接服务 <--result--- OpenClaw
 * 
 * 桥接服务负责:
 * 1. 接收 C++ 后端发来的用户消息 (POST :8765/process)
 * 2. 管理与 OpenClaw 的 session 会话
 * 3. 将 OpenClaw 的处理结果异步回调给 C++ 后端 (POST :8766/callback)
 */

require('dotenv').config();
const express = require('express');
const axios = require('axios');
const { v4: uuidv4 } = require('uuid');
const logger = require('./logger');
const SessionManager = require('./session');

// ==================== 配置 ====================

const CONFIG = {
    recvPort: parseInt(process.env.BRIDGE_RECV_PORT || '8765'),
    callbackPort: parseInt(process.env.BRIDGE_CALLBACK_PORT || '8766'),
    openclawEndpoint: process.env.OPENCLAW_ENDPOINT || 'http://localhost:8000',
    openclawApiKey: process.env.OPENCLAW_API_KEY || '',
    requestTimeoutMs: parseInt(process.env.REQUEST_TIMEOUT_MS || '30000'),
    sessionTtlSeconds: parseInt(process.env.SESSION_TTL_SECONDS || '3600'),
    backendCallbackUrl: process.env.BACKEND_CALLBACK_URL || 'http://localhost:8766',
};

// ==================== 会话管理 ====================

const sessionManager = new SessionManager(CONFIG.sessionTtlSeconds);

// ==================== 接收服务 (端口 8765) ====================
// 接收来自 C++ 后端的请求

const recvApp = express();
recvApp.use(express.json({ limit: '10mb' }));

/**
 * 健康检查
 */
recvApp.get('/health', (req, res) => {
    res.json({
        status: 'ok',
        service: 'yachiyo-openclaw-bridge',
        uptime: process.uptime(),
        activeSessions: sessionManager.getActiveCount(),
        openclawEndpoint: CONFIG.openclawEndpoint,
    });
});

/**
 * 处理用户消息
 * C++ 后端 POST 到这里，桥接服务转发给 OpenClaw
 * 
 * 请求体:
 * {
 *   "request_id": "uuid",
 *   "user_id": "user123",
 *   "text": "用户输入的文本",
 *   "context": "对话上下文",
 *   "emotion_hints": ["开心"],
 *   "max_tokens": 1000,
 *   "temperature": 0.7
 * }
 */
recvApp.post('/process', async (req, res) => {
    const startTime = Date.now();
    const requestId = req.body.request_id || uuidv4();
    const { user_id, text, context, emotion_hints, max_tokens, temperature } = req.body;

    logger.info(`[${requestId}] 收到处理请求: user=${user_id}, text="${text?.substring(0, 50)}..."`);

    if (!text) {
        return res.status(400).json({
            request_id: requestId,
            success: false,
            error: '缺少 text 字段',
        });
    }

    // 获取或创建会话
    const session = sessionManager.getOrCreate(user_id);

    // 追加用户消息到会话上下文
    session.addMessage('user', text);

    try {
        // 构建 OpenClaw 请求
        const openclawRequest = {
            session_id: session.id,
            messages: session.getMessages(),
            user_id: user_id,
            text: text,
            context: context || session.getContextSummary(),
            emotion_hints: emotion_hints || [],
            max_tokens: max_tokens || 1000,
            temperature: temperature || 0.7,
            metadata: {
                request_id: requestId,
                source: 'yachiyo-bridge',
                timestamp: new Date().toISOString(),
            },
        };

        // 发送到 OpenClaw
        logger.debug(`[${requestId}] 转发到 OpenClaw: ${CONFIG.openclawEndpoint}/process`);

        const openclawResponse = await axios.post(
            `${CONFIG.openclawEndpoint}/process`,
            openclawRequest,
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

        // 追加助手回复到会话
        if (result.text) {
            session.addMessage('assistant', result.text);
        }

        // 标准化响应格式
        const bridgeResponse = {
            request_id: requestId,
            success: true,
            text: result.text || '',
            emotions: result.emotions || [],
            actions: result.actions || [],
            processing_time_ms: processingTimeMs,
            session_id: session.id,
            metadata: {
                openclaw_processing_ms: result.processing_time_ms || 0,
                bridge_overhead_ms: processingTimeMs - (result.processing_time_ms || 0),
                message_count: session.getMessageCount(),
            },
        };

        logger.info(`[${requestId}] 处理完成 (${processingTimeMs}ms): "${result.text?.substring(0, 80)}..."`);

        // 同步返回结果给 C++ 后端
        res.json(bridgeResponse);

        // 同时异步推送到回调端口 (可选, 用于异步场景)
        pushToCallback(bridgeResponse).catch(err => {
            logger.debug(`[${requestId}] 回调推送跳过 (可选): ${err.message}`);
        });

    } catch (error) {
        const processingTimeMs = Date.now() - startTime;

        if (error.response) {
            // OpenClaw 返回了错误状态码
            logger.error(`[${requestId}] OpenClaw 返回错误: ${error.response.status} - ${JSON.stringify(error.response.data)}`);
            res.status(502).json({
                request_id: requestId,
                success: false,
                error: `OpenClaw 错误: ${error.response.status}`,
                detail: error.response.data,
                processing_time_ms: processingTimeMs,
            });
        } else if (error.code === 'ECONNREFUSED' || error.code === 'ETIMEDOUT') {
            // OpenClaw 不可达
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

/**
 * 获取会话信息
 */
recvApp.get('/session/:userId', (req, res) => {
    const session = sessionManager.get(req.params.userId);
    if (!session) {
        return res.status(404).json({ error: '会话不存在' });
    }
    res.json({
        session_id: session.id,
        user_id: req.params.userId,
        message_count: session.getMessageCount(),
        created_at: session.createdAt,
        last_active: session.lastActive,
    });
});

/**
 * 清除用户会话
 */
recvApp.delete('/session/:userId', (req, res) => {
    sessionManager.remove(req.params.userId);
    res.json({ success: true, message: '会话已清除' });
});

// ==================== 回调服务 (端口 8766) ====================
// 用于接收来自 OpenClaw 的异步回调 / C++ 后端也监听此端口

const callbackApp = express();
callbackApp.use(express.json({ limit: '10mb' }));

/**
 * 健康检查
 */
callbackApp.get('/health', (req, res) => {
    res.json({ status: 'ok', role: 'callback-receiver' });
});

/**
 * 接收 OpenClaw 异步回调
 * OpenClaw 可以主动 POST 结果到这里
 */
callbackApp.post('/callback', (req, res) => {
    const { request_id, text, emotions, actions } = req.body;
    logger.info(`[${request_id}] 收到 OpenClaw 异步回调`);

    // 转发给 C++ 后端 (如果 C++ 后端注册了回调监听)
    // 这里存储结果，等 C++ 后端来轮询或通过 WebSocket 推送
    callbackResults.set(request_id, {
        ...req.body,
        received_at: new Date().toISOString(),
    });

    res.json({ success: true, message: '回调已接收' });
});

/**
 * C++ 后端轮询获取异步结果
 */
callbackApp.get('/result/:requestId', (req, res) => {
    const result = callbackResults.get(req.params.requestId);
    if (!result) {
        return res.status(404).json({ error: '结果不存在或已过期' });
    }
    callbackResults.delete(req.params.requestId);
    res.json(result);
});

// 异步回调结果临时存储 (带自动过期)
const callbackResults = new Map();

// 每分钟清理过期结果
setInterval(() => {
    const now = Date.now();
    for (const [key, value] of callbackResults) {
        if (now - new Date(value.received_at).getTime() > 300000) { // 5分钟过期
            callbackResults.delete(key);
        }
    }
}, 60000);

// ==================== 异步推送到回调 ====================

async function pushToCallback(response) {
    await axios.post(`${CONFIG.backendCallbackUrl}/callback`, response, {
        timeout: 5000,
        headers: { 'Content-Type': 'application/json' },
    });
}

// ==================== 启动服务 ====================

recvApp.listen(CONFIG.recvPort, () => {
    logger.info(`📡 桥接服务 (接收端) 已启动: http://0.0.0.0:${CONFIG.recvPort}`);
    logger.info(`   POST /process  — C++ 后端发送消息到这里`);
    logger.info(`   GET  /health   — 健康检查`);
});

callbackApp.listen(CONFIG.callbackPort, () => {
    logger.info(`📡 桥接服务 (回调端) 已启动: http://0.0.0.0:${CONFIG.callbackPort}`);
    logger.info(`   POST /callback — OpenClaw 异步回调`);
    logger.info(`   GET  /result/:id — C++ 后端轮询结果`);
});

logger.info(`🔗 OpenClaw 端点: ${CONFIG.openclawEndpoint}`);
logger.info(`🔗 后端回调地址: ${CONFIG.backendCallbackUrl}`);

// 优雅退出
process.on('SIGTERM', () => {
    logger.info('收到 SIGTERM，正在关闭...');
    process.exit(0);
});

process.on('SIGINT', () => {
    logger.info('收到 SIGINT，正在关闭...');
    process.exit(0);
});
