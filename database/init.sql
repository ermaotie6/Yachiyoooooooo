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

-- ============ 消息表 ============

CREATE TABLE IF NOT EXISTS messages (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    
    -- 消息内容
    content TEXT NOT NULL,
    content_hash VARCHAR(255),  -- 用于去重
    
    -- 消息元数据
    language VARCHAR(10),
    character_count INT,
    
    -- 审核信息
    review_status VARCHAR(50) DEFAULT 'pending',  -- pending, approved, rejected
    moderation_result JSONB,
    moderation_timestamp TIMESTAMP,
    moderation_notes TEXT,
    
    -- 响应信息
    avatar_response JSONB,
    response_timestamp TIMESTAMP,
    
    -- 时间戳
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    -- 状态
    is_visible BOOLEAN DEFAULT TRUE
);

CREATE INDEX IF NOT EXISTS messages_user_id_idx ON messages(user_id);
CREATE INDEX IF NOT EXISTS messages_created_at_idx ON messages(created_at);
CREATE INDEX IF NOT EXISTS messages_review_status_idx ON messages(review_status);
CREATE INDEX IF NOT EXISTS messages_user_created_idx ON messages(user_id, created_at DESC);

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

-- ============ Avatar 响应缓存表 ============

CREATE TABLE IF NOT EXISTS avatar_responses (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    message_id BIGINT REFERENCES messages(id) ON DELETE CASCADE,
    
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
CREATE INDEX IF NOT EXISTS responses_message_id_idx ON avatar_responses(message_id);
CREATE INDEX IF NOT EXISTS responses_expires_at_idx ON avatar_responses(expires_at);
CREATE INDEX IF NOT EXISTS responses_request_hash_idx ON avatar_responses(request_hash);

-- ============ 审核结果表 ============

CREATE TABLE IF NOT EXISTS moderation_logs (
    id BIGSERIAL PRIMARY KEY,
    message_id BIGINT NOT NULL REFERENCES messages(id) ON DELETE CASCADE,
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

-- ============ 用户统计表 ============

CREATE TABLE IF NOT EXISTS user_statistics (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL UNIQUE REFERENCES users(id) ON DELETE CASCADE,
    
    -- 消息统计
    total_messages INT DEFAULT 0,
    total_characters INT DEFAULT 0,
    average_message_length DECIMAL(10, 2),
    
    -- 审核统计
    flagged_messages INT DEFAULT 0,
    violation_count INT DEFAULT 0,
    
    -- 活跃度统计
    first_message_date TIMESTAMP,
    last_message_date TIMESTAMP,
    days_active INT DEFAULT 0,
    
    -- 响应统计
    average_response_time_ms INT,
    fastest_response_ms INT,
    slowest_response_ms INT,
    
    -- 更新时间
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS stats_user_id_idx ON user_statistics(user_id);

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
    message_id BIGINT REFERENCES messages(id) ON DELETE SET NULL,
    
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
LEFT JOIN messages m ON u.id = m.user_id
WHERE u.is_active = TRUE AND u.is_banned = FALSE
GROUP BY u.id, u.username, u.email;

-- 最近消息视图
CREATE OR REPLACE VIEW recent_messages AS
SELECT m.id, m.user_id, u.username, m.content, m.review_status, m.created_at
FROM messages m
JOIN users u ON m.user_id = u.id
WHERE m.created_at > NOW() - INTERVAL '24 hours'
ORDER BY m.created_at DESC;

-- ============ 创建索引 ============

CREATE INDEX IF NOT EXISTS idx_messages_user_created ON messages(user_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_messages_review_status ON messages(review_status);
CREATE INDEX IF NOT EXISTS idx_contexts_active ON conversation_contexts(is_active) WHERE is_active = TRUE;
CREATE INDEX IF NOT EXISTS idx_moderation_severity ON moderation_logs(severity_score DESC);
CREATE INDEX IF NOT EXISTS idx_websocket_active ON websocket_logs(status) WHERE status = 'connected';

-- ============ 创建初始数据 ============

-- 创建管理员用户（可选）
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

DROP TRIGGER IF EXISTS messages_update_timestamp ON messages;
CREATE TRIGGER messages_update_timestamp
BEFORE UPDATE ON messages
FOR EACH ROW
EXECUTE FUNCTION update_timestamp();

DROP TRIGGER IF EXISTS contexts_update_timestamp ON conversation_contexts;
CREATE TRIGGER contexts_update_timestamp
BEFORE UPDATE ON conversation_contexts
FOR EACH ROW
EXECUTE FUNCTION update_timestamp();

DROP TRIGGER IF EXISTS stats_update_timestamp ON user_statistics;
CREATE TRIGGER stats_update_timestamp
BEFORE UPDATE ON user_statistics
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
