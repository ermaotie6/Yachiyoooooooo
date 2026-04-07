<template>
  <div class="chat-container">
    <div class="chat-header">
      <h1>AI 智能助手</h1>
      <el-button @click="startNewSession">新建会话</el-button>
    </div>

    <div class="chat-content">
      <div class="messages-container">
        <div
          v-for="message in messages"
          :key="message.id"
          :class="['message', message.type === 'user' ? 'user' : 'ai']"
        >
          <div class="message-avatar">
            {{ message.type === 'user' ? '👤' : '🤖' }}
          </div>
          <div class="message-body">
            <p>{{ message.content }}</p>
            <small>{{ formatTime(message.timestamp) }}</small>
          </div>
        </div>
        <div v-if="loading" class="message ai">
          <div class="message-avatar">🤖</div>
          <div class="message-body">
            <el-skeleton :rows="1" animated />
          </div>
        </div>
      </div>

      <div class="input-area">
        <el-input
          v-model="userMessage"
          type="textarea"
          :rows="3"
          placeholder="输入您的问题..."
          @keyup.ctrl.enter="sendMessage"
          @keyup.meta.enter="sendMessage"
        />
        <el-button type="primary" @click="sendMessage" :loading="loading">
          发送
        </el-button>
      </div>
    </div>

    <div class="chat-sidebar">
      <h3>会话历史</h3>
      <div class="sessions-list">
        <div
          v-for="session in sessions"
          :key="session.sessionId"
          :class="['session-item', { active: session.sessionId === currentSessionId }]"
          @click="switchSession(session.sessionId)"
        >
          <div class="session-name">{{ formatDate(session.createdAt) }}</div>
          <small>{{ session.messageCount }} 条消息</small>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { apiV2 } from '@/api/client'
import type { ChatMessage, ChatSession } from '@/types'

const messages = ref<Array<ChatMessage & { type: 'user' | 'ai' }>>([])
const sessions = ref<ChatSession[]>([])
const userMessage = ref('')
const loading = ref(false)
const currentSessionId = ref('')

const sendMessage = async () => {
  if (!userMessage.value.trim()) {
    ElMessage.warning('请输入内容')
    return
  }

  const message = userMessage.value
  userMessage.value = ''
  loading.value = true

  try {
    messages.value.push({
      id: Date.now(),
      type: 'user',
      content: message,
      sessionId: currentSessionId.value,
      userMessage: message,
      aiResponse: '',
      timestamp: new Date().toISOString(),
      processingTime: 0
    })

    const response = await apiV2.post('/ai/chat', {
      message,
      chat_id: currentSessionId.value
    })

    const data = response.data.data
    const aiText = data.response || data.aiResponse || ''
    const chatId = data.chat_id || data.sessionId || currentSessionId.value
    messages.value.push({
      id: Date.now() + 1,
      type: 'ai',
      content: aiText,
      sessionId: chatId,
      userMessage: message,
      aiResponse: aiText,
      timestamp: new Date().toISOString(),
      processingTime: data.response_time || data.processingTime || 0
    })

    currentSessionId.value = chatId
  } catch (error: any) {
    ElMessage.error(error.response?.data?.message || '发送失败')
  } finally {
    loading.value = false
  }
}

const startNewSession = () => {
  currentSessionId.value = `session_${Date.now()}`
  messages.value = []
  userMessage.value = ''
}

const switchSession = async (sessionId: string) => {
  currentSessionId.value = sessionId
  try {
    const response = await apiV2.get(`/ai/history?chat_id=${sessionId}`)
    // 后端返回 { chats: [{ messages: [...] }] }，提取消息并按 role 分类
    const chats = response.data.data?.chats || []
    const expandedMessages: Array<ChatMessage & { type: 'user' | 'ai'; content: string }> = []
    for (const chat of chats) {
      const chatMessages = chat.messages || []
      for (const msg of chatMessages) {
        expandedMessages.push({
          id: typeof msg.id === 'number' ? msg.id : Date.now(),
          sessionId: sessionId,
          userMessage: msg.role === 'user' ? msg.content : '',
          aiResponse: msg.role === 'assistant' ? msg.content : '',
          timestamp: msg.created_at || new Date().toISOString(),
          processingTime: 0,
          type: msg.role === 'user' ? 'user' : 'ai',
          content: msg.content || ''
        })
      }
    }
    messages.value = expandedMessages
  } catch (error) {
    ElMessage.error('加载会话失败')
  }
}

const formatTime = (time: string) => {
  return new Date(time).toLocaleTimeString('zh-CN')
}

const formatDate = (date: string) => {
  return new Date(date).toLocaleDateString('zh-CN')
}

onMounted(async () => {
  startNewSession()
  try {
    const response = await apiV2.get('/ai/sessions')
    if (response.data?.data?.chats) {
      // 后端返回 { chats: [...] }，映射为前端 ChatSession 格式
      sessions.value = response.data.data.chats.map((chat: any) => ({
        sessionId: chat.id || chat.chat_id,
        userId: 0,
        createdAt: chat.created_at || chat.createdAt || '',
        updatedAt: chat.updated_at || chat.updatedAt || '',
        messageCount: chat.message_count || chat.messageCount || 0
      }))
    } else if (response.data?.data) {
      sessions.value = response.data.data
    }
  } catch (error) {
    console.log('无会话历史')
  }
})
</script>

<style scoped>
.chat-container {
  display: grid;
  grid-template-columns: 1fr 250px;
  gap: 20px;
  height: calc(100vh - 200px);
  padding: 20px;
}

.chat-header {
  grid-column: 1 / -1;
  display: flex;
  justify-content: space-between;
  align-items: center;
  background: white;
  padding: 20px;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  margin-bottom: 20px;
}

.chat-content {
  display: flex;
  flex-direction: column;
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  overflow: hidden;
}

.messages-container {
  flex: 1;
  overflow-y: auto;
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 15px;
}

.message {
  display: flex;
  gap: 12px;
}

.message.user {
  justify-content: flex-end;
}

.message.ai {
  justify-content: flex-start;
}

.message-avatar {
  font-size: 24px;
  min-width: 30px;
  text-align: center;
}

.message-body {
  max-width: 70%;
  padding: 12px 16px;
  border-radius: 8px;
  background: #f0f0f0;
}

.message.user .message-body {
  background: #667eea;
  color: white;
}

.message-body p {
  margin: 0;
  word-wrap: break-word;
}

.message-body small {
  display: block;
  margin-top: 8px;
  opacity: 0.7;
  font-size: 12px;
}

.input-area {
  padding: 20px;
  border-top: 1px solid #eee;
  display: flex;
  gap: 10px;
}

.input-area :deep(.el-input__textarea) {
  height: auto;
}

.chat-sidebar {
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  padding: 15px;
  grid-row: 2 / 4;
  overflow-y: auto;
}

.chat-sidebar h3 {
  margin: 0 0 15px 0;
  font-size: 14px;
  color: #333;
}

.sessions-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.session-item {
  padding: 12px;
  border-radius: 6px;
  cursor: pointer;
  transition: background 0.3s;
  border: 1px solid #eee;
}

.session-item:hover {
  background: #f5f5f5;
}

.session-item.active {
  background: #667eea;
  color: white;
  border-color: #667eea;
}

.session-name {
  font-size: 13px;
  font-weight: 500;
}

.session-item small {
  display: block;
  margin-top: 4px;
  opacity: 0.7;
  font-size: 11px;
}

@media (max-width: 768px) {
  .chat-container {
    grid-template-columns: 1fr;
  }

  .chat-sidebar {
    grid-row: auto;
  }

  .message-body {
    max-width: 90%;
  }
}
</style>
