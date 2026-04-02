-- Yachiyo 虚拟主播直播平台 - 数据库初始化脚本
-- 版本: 2.0.0
-- 创建日期: 2026-04-01

-- 删除已存在的表（v1.0 社交媒体表）
DROP TABLE IF EXISTS comments CASCADE;
DROP TABLE IF EXISTS post_likes CASCADE;
DROP TABLE IF EXISTS post_favorites CASCADE;
DROP TABLE IF EXISTS posts CASCADE;
DROP TABLE IF EXISTS users CASCADE;
DROP TABLE IF EXISTS broadcaster_responses CASCADE;
DROP TABLE IF EXISTS user_messages CASCADE;
DROP TABLE IF EXISTS preset_actions CASCADE;
DROP TABLE IF EXISTS blocked_keywords CASCADE;
DROP TABLE IF EXISTS user_blacklist CASCADE;
DROP TABLE IF EXISTS review_logs CASCADE;
DROP TABLE IF EXISTS broadcaster_config CASCADE;

-- ============================================
-- 1. 用户表 (简化版: 普通用户 + 管理员)
-- ============================================
CREATE TABLE users (
    id BIGSERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    salt VARCHAR(32) NOT NULL,
    
    -- 用户信息
    nickname VARCHAR(50),
    avatar_url VARCHAR(255),
    bio TEXT,
    
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
    
    -- 元数据
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login_at TIMESTAMP,
    last_login_ip VARCHAR(45),
    
    INDEX idx_username (username),
    INDEX idx_email (email),
    INDEX idx_status (status),
    INDEX idx_is_banned (is_banned)
);

-- ============================================
-- 2. 虚拟主播配置表 (由openclaw控制)
-- ============================================
CREATE TABLE broadcaster_config (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
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
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    UNIQUE(name)
);

-- ============================================
-- 3. 用户消息表
-- ============================================
CREATE TABLE user_messages (
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
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_user_id (user_id),
    INDEX idx_review_status (review_status),
    INDEX idx_created_at (created_at),
    INDEX idx_is_spam (is_spam)
);

-- ============================================
-- 4. 虚拟形象反应表
-- ============================================
CREATE TABLE broadcaster_responses (
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
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_message_id (message_id),
    INDEX idx_created_at (created_at)
);

-- ============================================
-- 5. 预设动作库
-- ============================================
CREATE TABLE preset_actions (
    id SERIAL PRIMARY KEY,
    action_name VARCHAR(50) UNIQUE NOT NULL,
    action_description TEXT,
    animation_params JSONB,
    duration_ms INT,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_action_name (action_name),
    INDEX idx_is_active (is_active)
);

-- ============================================
-- 6. 敏感词库
-- ============================================
CREATE TABLE blocked_keywords (
    id SERIAL PRIMARY KEY,
    keyword VARCHAR(100) UNIQUE NOT NULL,
    -- 1=敏感词, 2=广告词, 3=骚扰词
    keyword_type SMALLINT,
    -- 1=低, 2=中, 3=高
    severity SMALLINT DEFAULT 1,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_keyword (keyword),
    INDEX idx_keyword_type (keyword_type),
    INDEX idx_is_active (is_active)
);

-- ============================================
-- 7. 用户黑名单 (IP/邮箱)
-- ============================================
CREATE TABLE user_blacklist (
    id BIGSERIAL PRIMARY KEY,
    identifier VARCHAR(255) UNIQUE NOT NULL,
    -- 1=IP, 2=邮箱
    identifier_type SMALLINT,
    reason TEXT,
    blocked_by BIGINT REFERENCES users(id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP,
    
    INDEX idx_identifier (identifier),
    INDEX idx_identifier_type (identifier_type),
    INDEX idx_expires_at (expires_at)
);

-- ============================================
-- 8. 审查日志
-- ============================================
CREATE TABLE review_logs (
    id BIGSERIAL PRIMARY KEY,
    message_id BIGINT NOT NULL REFERENCES user_messages(id) ON DELETE CASCADE,
    -- 1=自动, 2=人工
    review_type SMALLINT,
    reviewed_by BIGINT REFERENCES users(id),
    -- 1=通过, 2=拒绝, 3=隐藏
    action_taken SMALLINT,
    reason TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_message_id (message_id),
    INDEX idx_reviewed_by (reviewed_by),
    INDEX idx_created_at (created_at)
);

-- ============================================
-- 9. 初始化数据
-- ============================================

-- 插入虚拟主播配置
INSERT INTO broadcaster_config (
    name, description, default_language, is_active, is_streaming
) VALUES (
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

-- 插入初始用户（管理员）
INSERT INTO users (username, email, password_hash, salt, nickname, role, status)
VALUES (
    'openclaw',
    'admin@yachiyo.com',
    'placeholder_hash',
    'placeholder_salt',
    'System Admin',
    99,
    1
) ON CONFLICT (username) DO NOTHING;

-- ============================================
-- 10. 修改时间戳函数 (自动更新 updated_at)
-- ============================================
CREATE OR REPLACE FUNCTION update_timestamp()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- 创建触发器
DROP TRIGGER IF EXISTS users_update_timestamp ON users;
CREATE TRIGGER users_update_timestamp
    BEFORE UPDATE ON users
    FOR EACH ROW
    EXECUTE FUNCTION update_timestamp();

DROP TRIGGER IF EXISTS broadcaster_config_update_timestamp ON broadcaster_config;
CREATE TRIGGER broadcaster_config_update_timestamp
    BEFORE UPDATE ON broadcaster_config
    FOR EACH ROW
    EXECUTE FUNCTION update_timestamp();

-- ============================================
-- 11. 创建统计视图 (可选)
-- ============================================
CREATE OR REPLACE VIEW user_statistics AS
SELECT
    u.id,
    u.username,
    u.nickname,
    COUNT(DISTINCT m.id) AS total_messages,
    COUNT(DISTINCT CASE WHEN m.review_status = 1 THEN m.id END) AS approved_messages,
    COUNT(DISTINCT CASE WHEN m.review_status = 2 THEN m.id END) AS rejected_messages,
    COUNT(DISTINCT CASE WHEN m.is_spam = TRUE THEN m.id END) AS spam_count,
    u.warnings_count,
    u.is_banned,
    u.created_at,
    MAX(u.last_login_at) AS last_activity
FROM users u
LEFT JOIN user_messages m ON u.id = m.user_id
GROUP BY u.id, u.username, u.nickname, u.warnings_count, u.is_banned, u.created_at;

-- ============================================
-- 12. 索引优化
-- ============================================
CREATE INDEX idx_messages_user_created ON user_messages(user_id, created_at DESC);
CREATE INDEX idx_messages_review_created ON user_messages(review_status, created_at DESC);
CREATE INDEX idx_responses_message ON broadcaster_responses(message_id);
CREATE INDEX idx_responses_created ON broadcaster_responses(created_at DESC);
