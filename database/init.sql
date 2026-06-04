-- Yachiyo 虚拟直播平台数据库初始化脚本
-- PostgreSQL 版本

BEGIN;

-- ============ 用户表 ============

CREATE TABLE IF NOT EXISTS users (
    id BIGSERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    salt VARCHAR(32) NOT NULL,
    
    -- 用户信息
    nickname VARCHAR(50),
    avatar_url VARCHAR(255),
    bio TEXT,
    
    -- 用户个性化数据
    profile_data JSONB,
    preferences JSONB,
    
    -- 用户角色 (1=普通用户, 99=管理员)
    role SMALLINT DEFAULT 1 CHECK (role IN (1, 99)),
    -- 用户状态 (1=活跃, 2=禁用, 3=封禁)
    status SMALLINT DEFAULT 1 CHECK (status IN (1, 2, 3)),
    
    -- 安全统计
    messages_sent BIGINT DEFAULT 0,
    messages_approved BIGINT DEFAULT 0,
    messages_rejected BIGINT DEFAULT 0,
    warnings_count SMALLINT DEFAULT 0,
    is_banned BOOLEAN DEFAULT FALSE,
    ban_reason TEXT,
    ban_until TIMESTAMP,
    
    -- 时间戳
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login_at TIMESTAMP,
    last_login_ip VARCHAR(45),
    
    -- 状态
    is_active BOOLEAN DEFAULT TRUE
);

CREATE INDEX IF NOT EXISTS users_username_idx ON users(username);
CREATE INDEX IF NOT EXISTS users_email_idx ON users(email);

-- ============ 会话表 ============

CREATE TABLE IF NOT EXISTS sessions (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    
    -- 会话信息
    session_id VARCHAR(255) UNIQUE NOT NULL,
    access_token VARCHAR(1024),
    refresh_token VARCHAR(1024),
    
    -- 会话状态
    ip_address VARCHAR(45),
    user_agent TEXT,
    device_type VARCHAR(50),
    
    -- 时间戳
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP,
    
    -- 状态
    is_active BOOLEAN DEFAULT TRUE
);

CREATE INDEX IF NOT EXISTS sessions_user_id_idx ON sessions(user_id);
CREATE INDEX IF NOT EXISTS sessions_session_id_idx ON sessions(session_id);
CREATE INDEX IF NOT EXISTS sessions_expires_at_idx ON sessions(expires_at);

-- ============ 消息表（已废弃，统一使用 user_messages 表） ============
-- 注意: messages 表已移除，实际业务数据存储在 user_messages 表中。
-- 后端 MessageServiceImpl 仅操作 user_messages。

-- ============ 对话上下文表 ============

CREATE TABLE IF NOT EXISTS conversation_contexts (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    
    -- 会话标识
    session_id VARCHAR(255),
    conversation_id VARCHAR(255),
    
    -- 上下文数据
    context_data JSONB,  -- 存储完整的对话上下文
    message_history JSONB,  -- 消息历史摘要
    user_profile JSONB,  -- 用户个性化信息
    
    -- 统计信息
    message_count INT DEFAULT 0,
    total_characters INT DEFAULT 0,
    average_response_time_ms INT,
    
    -- 时间戳
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_accessed TIMESTAMP,
    
    -- 状态
    is_active BOOLEAN DEFAULT TRUE
);

CREATE INDEX IF NOT EXISTS contexts_user_id_idx ON conversation_contexts(user_id);
CREATE INDEX IF NOT EXISTS contexts_user_updated_idx ON conversation_contexts(user_id, updated_at DESC);
CREATE INDEX IF NOT EXISTS contexts_session_id_idx ON conversation_contexts(session_id);

-- ============ 用户消息表 (后端 MessageServiceImpl 引用) ============
-- 注意: 此表必须在 avatar_responses、moderation_logs 之前创建（外键依赖）

CREATE TABLE IF NOT EXISTS user_messages (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    
    -- 消息内容
    original_message TEXT NOT NULL,
    message_length INT NOT NULL,
    
    -- 审查状态 (0=未审查, 1=通过, 2=拒绝, 3=待人工审查)
    review_status SMALLINT DEFAULT 0 CHECK (review_status IN (0, 1, 2, 3)),
    review_reason VARCHAR(255),
    reviewed_by BIGINT REFERENCES users(id),
    reviewed_at TIMESTAMP,
    
    -- 恶意行为检测
    is_spam BOOLEAN DEFAULT FALSE,
    is_abusive BOOLEAN DEFAULT FALSE,
    is_blocked_keyword BOOLEAN DEFAULT FALSE,
    spam_score DECIMAL(5, 2) DEFAULT 0,
    
    -- 速率限制
    rate_limit_violated BOOLEAN DEFAULT FALSE,
    
    -- 元数据
    user_ip VARCHAR(45),
    user_agent VARCHAR(255),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_user_messages_user_id ON user_messages(user_id);
CREATE INDEX IF NOT EXISTS idx_user_messages_review_status ON user_messages(review_status);
CREATE INDEX IF NOT EXISTS idx_user_messages_created_at ON user_messages(created_at);
CREATE INDEX IF NOT EXISTS idx_user_messages_is_spam ON user_messages(is_spam);
CREATE INDEX IF NOT EXISTS idx_user_messages_user_created ON user_messages(user_id, created_at DESC);

-- ============ Avatar 响应缓存表 ============

CREATE TABLE IF NOT EXISTS avatar_responses (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    message_id BIGINT REFERENCES user_messages(id) ON DELETE CASCADE,
    
    -- 请求信息
    request_text TEXT,
    request_hash VARCHAR(255),
    
    -- 响应数据
    response_data JSONB,
    
    -- 生成参数
    emotion_analysis JSONB,
    animation_parameters JSONB,
    voice_parameters JSONB,
    
    -- 时间戳
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP,
    
    -- 状态
    is_cached BOOLEAN DEFAULT TRUE,
    hit_count INT DEFAULT 0
);

CREATE INDEX IF NOT EXISTS responses_user_id_idx ON avatar_responses(user_id);
CREATE UNIQUE INDEX IF NOT EXISTS responses_message_id_unique ON avatar_responses(message_id);
CREATE INDEX IF NOT EXISTS responses_message_id_idx ON avatar_responses(message_id);
CREATE INDEX IF NOT EXISTS responses_expires_at_idx ON avatar_responses(expires_at);
CREATE INDEX IF NOT EXISTS responses_request_hash_idx ON avatar_responses(request_hash);

-- ============ 审核结果表 ============

CREATE TABLE IF NOT EXISTS moderation_logs (
    id BIGSERIAL PRIMARY KEY,
    message_id BIGINT NOT NULL REFERENCES user_messages(id) ON DELETE CASCADE,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    
    -- 审核信息
    violation_type VARCHAR(50),  -- violence, adult, harassment, spam, hate, profanity
    severity_score DECIMAL(3, 2),  -- 0.0-1.0
    is_violation BOOLEAN,
    
    -- 审核细节
    violation_details JSONB,
    confidence_score DECIMAL(3, 2),
    
    -- 审核员信息
    reviewed_by VARCHAR(255),  -- 可选，如果是手动审核
    review_notes TEXT,
    
    -- 处理结果
    action_taken VARCHAR(50),  -- block, warning, none
    action_details JSONB,
    
    -- 时间戳
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    reviewed_at TIMESTAMP
);

CREATE INDEX IF NOT EXISTS moderation_message_idx ON moderation_logs(message_id);
CREATE INDEX IF NOT EXISTS moderation_user_idx ON moderation_logs(user_id);
CREATE INDEX IF NOT EXISTS moderation_severity_idx ON moderation_logs(severity_score);
CREATE INDEX IF NOT EXISTS moderation_created_idx ON moderation_logs(created_at DESC);

-- ============ 虚拟主播配置表 ============

CREATE TABLE IF NOT EXISTS broadcaster_config (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL,
    description TEXT,
    avatar_model_path VARCHAR(255),
    
    -- openclaw 配置
    openclaw_api_endpoint VARCHAR(255),
    openclaw_api_key VARCHAR(255),
    
    -- 语言与翻译
    default_language VARCHAR(10) DEFAULT 'ja',
    
    -- 状态
    is_active BOOLEAN DEFAULT TRUE,
    is_streaming BOOLEAN DEFAULT FALSE,
    
    -- 限制设置
    max_concurrent_users INT DEFAULT 1000,
    message_processing_timeout INT DEFAULT 5000,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- ============ 虚拟形象反应表 ============

CREATE TABLE IF NOT EXISTS broadcaster_responses (
    id BIGSERIAL PRIMARY KEY,
    message_id BIGINT NOT NULL REFERENCES user_messages(id) ON DELETE CASCADE,
    
    -- 响应内容
    original_message TEXT,
    
    -- openclaw 输出
    openclaw_response JSONB,
    speech_text VARCHAR(500),
    
    -- 动作列表
    actions JSONB,
    
    -- 语音生成
    tts_audio_url VARCHAR(255),
    tts_generated_at TIMESTAMP,
    
    -- 元数据
    processing_time_ms INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_responses_message_id ON broadcaster_responses(message_id);
CREATE INDEX IF NOT EXISTS idx_responses_created_at ON broadcaster_responses(created_at);

-- ============ 预设动作库 ============

CREATE TABLE IF NOT EXISTS preset_actions (
    id SERIAL PRIMARY KEY,
    action_name VARCHAR(50) UNIQUE NOT NULL,
    action_description TEXT,
    animation_params JSONB,
    duration_ms INT,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_preset_actions_name ON preset_actions(action_name);
CREATE INDEX IF NOT EXISTS idx_preset_actions_active ON preset_actions(is_active);

-- ============ 敏感词库 ============

CREATE TABLE IF NOT EXISTS blocked_keywords (
    id SERIAL PRIMARY KEY,
    keyword VARCHAR(100) UNIQUE NOT NULL,
    -- 1=敏感词, 2=广告词, 3=骚扰词
    keyword_type SMALLINT,
    -- 1=低, 2=中, 3=高
    severity SMALLINT DEFAULT 1,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_blocked_keywords_keyword ON blocked_keywords(keyword);
CREATE INDEX IF NOT EXISTS idx_blocked_keywords_type ON blocked_keywords(keyword_type);
CREATE INDEX IF NOT EXISTS idx_blocked_keywords_active ON blocked_keywords(is_active);

-- ============ 用户黑名单 (IP/邮箱) ============

CREATE TABLE IF NOT EXISTS user_blacklist (
    id BIGSERIAL PRIMARY KEY,
    identifier VARCHAR(255) UNIQUE NOT NULL,
    -- 1=IP, 2=邮箱
    identifier_type SMALLINT,
    reason TEXT,
    blocked_by BIGINT REFERENCES users(id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_blacklist_identifier ON user_blacklist(identifier);
CREATE INDEX IF NOT EXISTS idx_blacklist_type ON user_blacklist(identifier_type);
CREATE INDEX IF NOT EXISTS idx_blacklist_expires ON user_blacklist(expires_at);

-- ============ 审查日志 ============

CREATE TABLE IF NOT EXISTS review_logs (
    id BIGSERIAL PRIMARY KEY,
    message_id BIGINT NOT NULL REFERENCES user_messages(id) ON DELETE CASCADE,
    -- 1=自动, 2=人工
    review_type SMALLINT,
    reviewed_by BIGINT REFERENCES users(id),
    -- 1=通过, 2=拒绝, 3=隐藏
    action_taken SMALLINT,
    reason TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_review_logs_message_id ON review_logs(message_id);
CREATE INDEX IF NOT EXISTS idx_review_logs_reviewed_by ON review_logs(reviewed_by);
CREATE INDEX IF NOT EXISTS idx_review_logs_created_at ON review_logs(created_at);

-- ============ 用户统计视图（自动聚合，无需手动更新） ============

CREATE OR REPLACE VIEW user_statistics_view AS
SELECT
    u.id AS user_id,
    u.username,
    u.nickname,
    COUNT(DISTINCT m.id) AS total_messages,
    COALESCE(SUM(m.message_length), 0) AS total_characters,
    CASE WHEN COUNT(m.id) > 0 
         THEN ROUND(AVG(m.message_length)::numeric, 2) 
         ELSE 0 END AS average_message_length,
    COUNT(DISTINCT CASE WHEN m.review_status = 1 THEN m.id END) AS approved_messages,
    COUNT(DISTINCT CASE WHEN m.review_status = 2 THEN m.id END) AS rejected_messages,
    COUNT(DISTINCT CASE WHEN m.is_spam = TRUE THEN m.id END) AS spam_count,
    u.warnings_count,
    u.is_banned,
    MIN(m.created_at) AS first_message_date,
    MAX(m.created_at) AS last_message_date,
    u.created_at
FROM users u
LEFT JOIN user_messages m ON u.id = m.user_id
GROUP BY u.id, u.username, u.nickname, u.warnings_count, u.is_banned, u.created_at;

-- ============ WebSocket 连接日志表 ============

CREATE TABLE IF NOT EXISTS websocket_logs (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT REFERENCES users(id) ON DELETE SET NULL,
    
    -- 连接信息
    client_id BIGINT,
    connection_type VARCHAR(50),  -- web, mobile, desktop
    ip_address VARCHAR(45),
    user_agent TEXT,
    
    -- 连接状态
    status VARCHAR(50),  -- connected, disconnected, error
    error_message TEXT,
    
    -- 时间戳
    connected_at TIMESTAMP,
    disconnected_at TIMESTAMP,
    duration_ms INT,
    
    -- 统计
    messages_sent INT DEFAULT 0,
    messages_received INT DEFAULT 0
);

CREATE INDEX IF NOT EXISTS websocket_user_id_idx ON websocket_logs(user_id);
CREATE INDEX IF NOT EXISTS websocket_status_idx ON websocket_logs(status);
CREATE INDEX IF NOT EXISTS websocket_connected_idx ON websocket_logs(connected_at DESC);

-- ============ 系统日志表 ============

CREATE TABLE IF NOT EXISTS system_logs (
    id BIGSERIAL PRIMARY KEY,
    
    -- 日志级别
    level VARCHAR(20),  -- DEBUG, INFO, WARNING, ERROR, CRITICAL
    
    -- 日志内容
    component VARCHAR(100),
    message TEXT,
    details JSONB,
    
    -- 关联信息
    user_id BIGINT REFERENCES users(id) ON DELETE SET NULL,
    message_id BIGINT REFERENCES user_messages(id) ON DELETE SET NULL,
    
    -- 时间戳
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS logs_level_idx ON system_logs(level);
CREATE INDEX IF NOT EXISTS logs_component_idx ON system_logs(component);
CREATE INDEX IF NOT EXISTS logs_timestamp_idx ON system_logs(timestamp DESC);
CREATE INDEX IF NOT EXISTS logs_user_idx ON system_logs(user_id);

-- ============ 创建视图 ============

-- 活跃用户视图
CREATE OR REPLACE VIEW active_users AS
SELECT u.id, u.username, u.email, COUNT(m.id) as message_count, MAX(m.created_at) as last_activity
FROM users u
LEFT JOIN user_messages m ON u.id = m.user_id
WHERE u.is_active = TRUE AND u.is_banned = FALSE
GROUP BY u.id, u.username, u.email;

-- 最近消息视图
CREATE OR REPLACE VIEW recent_messages AS
SELECT m.id, m.user_id, u.username, m.original_message AS content, m.review_status, m.created_at
FROM user_messages m
JOIN users u ON m.user_id = u.id
WHERE m.created_at > NOW() - INTERVAL '24 hours'
ORDER BY m.created_at DESC;

-- ============ 创建索引 ============

CREATE INDEX IF NOT EXISTS idx_user_messages_review_created ON user_messages(review_status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_contexts_active ON conversation_contexts(is_active) WHERE is_active = TRUE;
CREATE INDEX IF NOT EXISTS idx_moderation_severity ON moderation_logs(severity_score DESC);
CREATE INDEX IF NOT EXISTS idx_websocket_active ON websocket_logs(status) WHERE status = 'connected';

-- ============ 创建初始数据 ============

-- 创建管理员用户（可选）
-- ⚠️ 警告: 以下密码哈希为 placeholder，无法用于登录！
-- 生产环境请通过 API 注册用户或手动生成正确的 bcrypt 哈希替换。
INSERT INTO users (username, email, password_hash, salt, nickname, role, status, is_active)
VALUES (
    'admin',
    'admin@yachiyo.local',
    '$2b$12$placeholder_hash_admin_user',
    'placeholder_salt',
    'System Admin',
    99,
    1,
    TRUE
) ON CONFLICT (username) DO NOTHING;

-- 创建测试用户（可选）
INSERT INTO users (username, email, password_hash, salt, nickname, role, status, is_active)
VALUES (
    'test_user',
    'test@yachiyo.local',
    '$2b$12$placeholder_hash_test_user',
    'placeholder_salt',
    'Test User',
    1,
    1,
    TRUE
) ON CONFLICT (username) DO NOTHING;

-- 插入虚拟主播配置
INSERT INTO broadcaster_config (name, description, default_language, is_active, is_streaming)
VALUES (
    'Yachiyo',
    '虚拟主播 Yachiyo',
    'ja',
    TRUE,
    FALSE
) ON CONFLICT (name) DO NOTHING;

-- 插入预设动作
INSERT INTO preset_actions (action_name, action_description, duration_ms, is_active)
VALUES
    ('bow', '鞠躬，向观众表示感谢或问候', 1000, TRUE),
    ('wave', '挥手，向观众打招呼', 500, TRUE),
    ('nod', '点头，表示同意或理解', 300, TRUE),
    ('smile', '微笑，表示开心或满足', 800, TRUE),
    ('surprised', '惊讶，表示意外或惊喜', 600, TRUE),
    ('thinking', '思考，做思考状态', 1200, TRUE),
    ('dance', '跳舞，庆祝或娱乐', 2000, TRUE),
    ('clap', '鼓掌，表示赞同或庆祝', 1500, TRUE),
    ('heart', '比心，表示爱意或感谢', 800, TRUE),
    ('thumbs_up', '竖起大拇指，表示赞好', 600, TRUE)
ON CONFLICT (action_name) DO NOTHING;

-- 插入常见敏感词 (示例)
INSERT INTO blocked_keywords (keyword, keyword_type, severity, is_active)
VALUES
    ('色情', 1, 3, TRUE),
    ('暴力', 1, 3, TRUE),
    ('骚扰', 1, 2, TRUE),
    ('广告', 2, 2, TRUE),
    ('微信', 2, 2, TRUE),
    ('QQ号', 2, 2, TRUE),
    ('骂人', 3, 2, TRUE),
    ('诈骗', 1, 3, TRUE)
ON CONFLICT (keyword) DO NOTHING;

-- ============ 创建函数 ============

-- 自动更新 updated_at 时间戳的触发器函数
CREATE OR REPLACE FUNCTION update_timestamp()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- 为各表创建触发器（先删除再创建，确保幂等执行）
DROP TRIGGER IF EXISTS users_update_timestamp ON users;
CREATE TRIGGER users_update_timestamp
BEFORE UPDATE ON users
FOR EACH ROW
EXECUTE FUNCTION update_timestamp();

DROP TRIGGER IF EXISTS sessions_update_timestamp ON sessions;
CREATE TRIGGER sessions_update_timestamp
BEFORE UPDATE ON sessions
FOR EACH ROW
EXECUTE FUNCTION update_timestamp();

DROP TRIGGER IF EXISTS contexts_update_timestamp ON conversation_contexts;
CREATE TRIGGER contexts_update_timestamp
BEFORE UPDATE ON conversation_contexts
FOR EACH ROW
EXECUTE FUNCTION update_timestamp();

DROP TRIGGER IF EXISTS broadcaster_config_update_timestamp ON broadcaster_config;
CREATE TRIGGER broadcaster_config_update_timestamp
BEFORE UPDATE ON broadcaster_config
FOR EACH ROW
EXECUTE FUNCTION update_timestamp();

-- ============ 权限和安全 ============

-- 创建应用用户（用于生产环境）
-- CREATE ROLE yachiyo_app WITH LOGIN PASSWORD 'secure_password';
-- GRANT CONNECT ON DATABASE yachiyo TO yachiyo_app;
-- GRANT USAGE ON SCHEMA public TO yachiyo_app;
-- GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO yachiyo_app;
-- GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO yachiyo_app;

COMMIT;
