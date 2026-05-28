<template>
  <section class="chat-section">
    <!-- 消息历史 -->
    <div class="message-history" ref="historyRef">
      <div v-for="msg in messages" :key="msg.id" :class="['message', `message-${msg.role}`]">
        <!-- 用户消息 -->
        <div v-if="msg.role === 'user'" class="user-message">
          <div class="sender-name">{{ msg.senderName || '匿名用户' }}</div>
          <div class="content">{{ msg.text }}</div>
          <div class="timestamp">{{ formatTime(msg.timestamp) }}</div>
        </div>

        <!-- Avatar 响应 -->
        <div v-if="msg.role === 'avatar'" class="avatar-message">
          <div class="sender-name">Yachiyo</div>
          <div class="content">{{ msg.text }}</div>

          <div v-if="msg.emotions && msg.emotions.length > 0" class="emotion-tags">
            <span v-for="emotion in msg.emotions" :key="emotion" class="emotion-tag">{{ emotion }}</span>
          </div>

          <div v-if="msg.audioUrl" class="audio-status">
            <span class="audio-indicator" :class="{ playing: msg.isAudioPlaying }">🔊</span>
            <span class="duration">{{ formatDuration(msg.audioDuration) }}</span>
          </div>

          <div class="timestamp">{{ formatTime(msg.timestamp) }}</div>
        </div>

        <!-- 系统消息 -->
        <div v-if="msg.role === 'system'" class="system-message">{{ msg.text }}</div>
      </div>

      <div v-if="isProcessing" class="loading-indicator">
        <div class="spinner"></div>
        <span>处理中...</span>
      </div>
    </div>

    <!-- 消息输入区域 -->
    <div class="input-section">
      <div class="input-wrapper">
        <div v-if="!isLoggedIn" class="login-hint" @click="$emit('loginRequired')">
          🔐 请先登录后再发送消息
        </div>
        <textarea
          v-model="inputValue"
          placeholder="输入你的消息（最多50字）..."
          :disabled="disabled"
          @keydown.ctrl.enter="$emit('send')"
          @input="onInputLimit"
          class="message-input"
        />
        <div class="char-counter">
          <span :class="{ warning: charCount > 40, danger: charCount > 45 }">
            {{ charCount }}/50
          </span>
        </div>
      </div>

      <div class="action-buttons">
        <!-- Emoji 选择器 -->
        <div class="emoji-picker-wrapper">
          <button
            @click="showEmojiPicker = !showEmojiPicker"
            class="emoji-toggle-btn"
            :class="{ active: showEmojiPicker }"
            :disabled="disabled"
            title="表情"
          >😊</button>
          <div v-if="showEmojiPicker" class="emoji-panel">
            <span
              v-for="emoji in emojiList"
              :key="emoji"
              class="emoji-item"
              @click="insertEmoji(emoji)"
            >{{ emoji }}</span>
          </div>
        </div>

        <button
          @click="$emit('send')"
          :disabled="disabled || !inputValue.trim()"
          class="send-button"
        >
          <span v-if="!isProcessing">发送</span>
          <span v-else><span class="button-spinner"></span>处理中</span>
        </button>

        <button
          @click="$emit('clear')"
          :disabled="messages.length === 0"
          class="clear-button"
          title="清空聊天历史"
        >清空</button>
      </div>
    </div>

    <!-- 连接提示 -->
    <div v-if="!isConnected" class="connection-hint">
      <span v-if="connectionError" class="error-message">❌ {{ connectionError }}</span>
      <span v-else class="info-message">⏳ 尝试连接中...</span>
    </div>
  </section>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import type { Message } from '@/composables/useChatMessages'

const props = defineProps<{
  messages: Message[]
  isConnected: boolean
  isProcessing: boolean
  connectionError: string | null
  disabled: boolean
  isLoggedIn: boolean
}>()

const emit = defineEmits<{
  send: []
  clear: []
  'update:inputValue': [value: string]
  loginRequired: []
}>()

const historyRef = ref<HTMLElement | null>(null)
const inputValue = ref('')
const showEmojiPicker = ref(false)

const emojiList = [
  '😊', '😂', '🤣', '❤️', '👍', '👋', '🎉', '🌸',
  '😍', '🤔', '😢', '😡', '😱', '😳', '🤩', '🥺',
  '💛', '💚', '💙', '🖤', '🔥', '✨', '🌟', '🌞',
  '🐶', '🐱', '🍀', '🍒', '🍰', '☕', '🌺', '🌻'
]

const charCount = computed(() => Array.from(inputValue.value).length)

const onInputLimit = () => {
  const chars = Array.from(inputValue.value)
  if (chars.length > 50) {
    inputValue.value = chars.slice(0, 50).join('')
  }
}

const insertEmoji = (emoji: string) => {
  if (Array.from(inputValue.value).length < 50) {
    inputValue.value += emoji
  }
  showEmojiPicker.value = false
}

// 向父组件同步 inputValue
const syncInput = () => emit('update:inputValue', inputValue.value)

const formatTime = (timestamp: number): string => {
  const d = new Date(timestamp)
  return `${String(d.getHours()).padStart(2, '0')}:${String(d.getMinutes()).padStart(2, '0')}`
}

const formatDuration = (ms?: number): string => {
  if (!ms || isNaN(ms) || ms <= 0) return '0:00'
  const s = Math.floor(ms / 1000)
  return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`
}

defineExpose({ historyRef, inputValue, clearInput: () => { inputValue.value = '' } })
</script>

<style scoped>
.chat-section {
  display: flex;
  flex-direction: column;
  background: rgba(255, 255, 255, 0.95);
  border-radius: 15px;
  overflow: hidden;
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.08);
  min-height: 0;
}

.message-history {
  flex: 1;
  min-height: 0;
  overflow-y: auto;
  padding: 15px;
}

.message {
  margin-bottom: 15px;
}

.user-message, .avatar-message {
  max-width: 80%;
  padding: 10px 15px;
  border-radius: 12px;
  font-size: 14px;
  line-height: 1.5;
}

.message-user {
  display: flex;
  justify-content: flex-end;
}

.user-message {
  background: #667eea;
  color: white;
  border-bottom-right-radius: 4px;
}

.message-avatar {
  display: flex;
  justify-content: flex-start;
}

.avatar-message {
  background: #f0f0f0;
  border-bottom-left-radius: 4px;
}

.sender-name {
  font-size: 12px;
  font-weight: bold;
  opacity: 0.8;
  margin-bottom: 4px;
}

.emotion-tags {
  margin-top: 6px;
  display: flex;
  gap: 4px;
  flex-wrap: wrap;
}

.emotion-tag {
  font-size: 11px;
  padding: 2px 8px;
  border-radius: 12px;
  background: rgba(102, 126, 234, 0.15);
  color: #667eea;
}

.audio-status {
  margin-top: 6px;
  font-size: 12px;
  color: #999;
}

.audio-indicator.playing {
  animation: pulse 1s infinite;
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.3; }
}

.timestamp {
  font-size: 11px;
  opacity: 0.6;
  margin-top: 4px;
  text-align: right;
}

.system-message {
  text-align: center;
  font-size: 12px;
  color: #999;
}

.loading-indicator {
  text-align: center;
  padding: 10px;
  color: #667eea;
}

.spinner {
  display: inline-block;
  width: 16px;
  height: 16px;
  border: 2px solid rgba(102, 126, 234, 0.3);
  border-top-color: #667eea;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
  margin-right: 6px;
  vertical-align: middle;
}

@keyframes spin { to { transform: rotate(360deg); } }

.button-spinner {
  display: inline-block;
  width: 14px;
  height: 14px;
  border: 2px solid rgba(255, 255, 255, 0.3);
  border-top-color: white;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

.input-section {
  flex-shrink: 0;
  border-top: 1px solid #eee;
  padding: 12px 15px;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.login-hint {
  padding: 10px 12px;
  background: #fff3e0;
  border: 1px dashed #ff9800;
  border-radius: 8px;
  font-size: 13px;
  color: #e65100;
  text-align: center;
  cursor: pointer;
  transition: background 0.2s;
}

.login-hint:hover { background: #ffe0b2; }

.input-wrapper {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.message-input {
  padding: 12px;
  border: 2px solid #e0e0e0;
  border-radius: 8px;
  font-family: inherit;
  font-size: 14px;
  resize: none;
  height: 60px;
  transition: border-color 0.3s;
}

.message-input:focus {
  outline: none;
  border-color: #667eea;
  box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
}

.message-input:disabled {
  background-color: #f5f5f5;
  color: #ccc;
  cursor: not-allowed;
}

.char-counter {
  font-size: 12px;
  color: #999;
  text-align: right;
}

.char-counter span.warning { color: #ff9800; font-weight: bold; }
.char-counter span.danger { color: #f44336; font-weight: bold; }

.action-buttons {
  display: flex;
  gap: 10px;
  align-items: flex-end;
}

.emoji-picker-wrapper { position: relative; }

.emoji-toggle-btn {
  width: 40px;
  height: 40px;
  border: 2px solid #e0e0e0;
  border-radius: 8px;
  background: white;
  font-size: 20px;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all 0.2s;
}

.emoji-toggle-btn:hover:not(:disabled) { border-color: #667eea; background: #f5f3ff; }
.emoji-toggle-btn.active { border-color: #667eea; background: #ede9fe; }
.emoji-toggle-btn:disabled { opacity: 0.5; cursor: not-allowed; }

.emoji-panel {
  position: absolute;
  bottom: 48px;
  left: 0;
  width: 340px;
  max-height: 260px;
  overflow-y: auto;
  background: white;
  border: 1px solid #e0e0e0;
  border-radius: 12px;
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.15);
  padding: 12px;
  display: grid;
  grid-template-columns: repeat(8, 1fr);
  gap: 6px;
  z-index: 999;
  animation: emojiSlideIn 0.2s ease-out;
}

@keyframes emojiSlideIn {
  from { opacity: 0; transform: translateY(8px); }
  to { opacity: 1; transform: translateY(0); }
}

.emoji-item {
  width: 38px;
  height: 38px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 24px;
  cursor: pointer;
  border-radius: 6px;
  transition: background 0.15s;
}

.emoji-item:hover { background: #f0f0f0; transform: scale(1.2); }

.send-button, .clear-button {
  padding: 12px 24px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border: none;
  border-radius: 8px;
  cursor: pointer;
  font-weight: 600;
  font-size: 14px;
  transition: all 0.3s;
  display: flex;
  align-items: center;
  gap: 6px;
  white-space: nowrap;
}

.send-button:hover:not(:disabled) { transform: translateY(-2px); box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4); }
.send-button:active:not(:disabled) { transform: translateY(0); }
.send-button:disabled, .clear-button:disabled { opacity: 0.5; cursor: not-allowed; }

.clear-button {
  background: linear-gradient(135deg, #e0e0e0 0%, #bdbdbd 100%);
  color: #666;
  padding: 10px 16px;
}

.connection-hint {
  flex-shrink: 0;
  padding: 10px;
  text-align: center;
  font-size: 12px;
  background: #fff9c4;
  border-top: 1px solid #eee;
}

.error-message { color: #d32f2f; }
.info-message { color: #f57f17; }

@media (max-width: 1200px) { .chat-section { min-height: 400px; } }

@media (max-width: 768px) {
  .user-message, .avatar-message { max-width: 90%; }
  .input-section { padding: 10px; gap: 5px; }
  .message-input { padding: 8px; font-size: 13px; }
  .send-button { padding: 8px 16px; font-size: 12px; }
  .emoji-panel {
    width: 280px;
    grid-template-columns: repeat(7, 1fr);
  }
}
</style>
