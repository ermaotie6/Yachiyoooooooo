/**
 * 会话管理器
 * 管理用户与 OpenClaw 之间的对话会话
 */

class Session {
    constructor(userId) {
        this.id = `session_${userId}_${Date.now()}`;
        this.userId = userId;
        this.messages = [];
        this.createdAt = new Date().toISOString();
        this.lastActive = new Date().toISOString();
        this.metadata = {};
    }

    addMessage(role, content) {
        this.messages.push({
            role,
            content,
            timestamp: new Date().toISOString(),
        });
        this.lastActive = new Date().toISOString();

        // 限制消息历史长度 (保留最近 40 条)
        if (this.messages.length > 40) {
            this.messages = this.messages.slice(-40);
        }
    }

    getMessages() {
        return this.messages;
    }

    getMessageCount() {
        return this.messages.length;
    }

    getContextSummary() {
        // 返回最近 5 条消息作为上下文摘要
        const recent = this.messages.slice(-5);
        return recent.map(m => `${m.role}: ${m.content}`).join('\n');
    }
}

class SessionManager {
    constructor(ttlSeconds = 3600) {
        this.sessions = new Map();
        this.ttlMs = ttlSeconds * 1000;

        // 每分钟清理过期会话
        this.cleanupInterval = setInterval(() => this.cleanup(), 60000);
    }

    getOrCreate(userId) {
        let session = this.sessions.get(userId);
        if (!session) {
            session = new Session(userId);
            this.sessions.set(userId, session);
        }
        session.lastActive = new Date().toISOString();
        return session;
    }

    get(userId) {
        return this.sessions.get(userId) || null;
    }

    remove(userId) {
        this.sessions.delete(userId);
    }

    getActiveCount() {
        return this.sessions.size;
    }

    cleanup() {
        const now = Date.now();
        for (const [userId, session] of this.sessions) {
            const lastActive = new Date(session.lastActive).getTime();
            if (now - lastActive > this.ttlMs) {
                this.sessions.delete(userId);
            }
        }
    }

    destroy() {
        clearInterval(this.cleanupInterval);
        this.sessions.clear();
    }
}

module.exports = SessionManager;
