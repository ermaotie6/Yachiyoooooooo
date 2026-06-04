<template>
  <section class="chat-section">
    <!-- 消息历史 -->
    <div class="message-history" ref="historyRef">
      <div v-for="msg in messages" :key="msg.id" :class="['message', `message-${msg.role}`]">
        <!-- 用户消息 -->
        <div v-if="msg.role === 'user'" class="bubble">
          <div class="sender-name">{{ msg.senderName || '匿名用户' }}</div>
          <div class="content">{{ msg.text }}</div>
          <div class="timestamp">{{ formatTime(msg.timestamp) }}</div>
        </div>

        <!-- Avatar 响应 -->
        <div v-if="msg.role === 'avatar' || msg.role === 'assistant'" class="bubble">
          <div class="sender-name">Yachiyo</div>
          <div class="content">{{ msg.text }}</div>
          <div v-if="msg.emotions && msg.emotions.length > 0" class="emotion-tags">
            <span v-for="emotion in msg.emotions" :key="emotion" class="emotion-tag">{{ emotion }}</span>
          </div>
          <div v-if="msg.audioUrl" class="audio-status">
            <span class="audio-indicator" :class="{ playing: msg.isAudioPlaying }">🔊</span>
            <span>{{ formatDuration(msg.audioDuration) }}</span>
          </div>
          <div class="timestamp">{{ formatTime(msg.timestamp) }}</div>
        </div>

        <!-- 系统消息 -->
        <div v-if="msg.role === 'system'" class="system-message">{{ msg.text }}</div>
      </div>

      <div v-if="isProcessing" class="loading-indicator">
        <div class="typing-dots">
          <span></span><span></span><span></span>
        </div>
        <span>八千代思考中...</span>
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
          placeholder="输入你的消息（回车发送，Shift+回车换行）..."
          :disabled="disabled"
          @keydown.enter.exact.prevent="$emit('send')"
          @keydown.shift.enter=""
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
          {{ isProcessing ? '发送中...' : '发送' }}
        </button>
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

const formatTime = (timestamp: number | string): string => {
  if (!timestamp) return '--:--'
  let ts: number
  if (typeof timestamp === 'string') {
    // 先尝试 ISO 8601 格式
    const parsed = Date.parse(timestamp)
    if (!isNaN(parsed)) {
      ts = parsed
    } else {
      // 可能是纯数字字符串 (如 "1780545489")
      const num = Number(timestamp)
      if (!isNaN(num) && num > 0) {
        ts = num > 1e12 ? num : num * 1000
      } else {
        return '--:--'
      }
    }
  } else {
    // Unix 秒 → 毫秒 (如果已经是毫秒级别，自动检测)
    ts = timestamp > 1e12 ? timestamp : timestamp * 1000
  }
  const d = new Date(ts)
  if (isNaN(d.getTime())) return '--:--'
  const month = String(d.getMonth() + 1).padStart(2, '0')
  const day = String(d.getDate()).padStart(2, '0')
  const hours = String(d.getHours()).padStart(2, '0')
  const minutes = String(d.getMinutes()).padStart(2, '0')
  const today = new Date()
  const isToday = d.getFullYear() === today.getFullYear() &&
    d.getMonth() === today.getMonth() &&
    d.getDate() === today.getDate()
  return isToday ? `${hours}:${minutes}` : `${month}/${day} ${hours}:${minutes}`
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
  background: var(--c-surface);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-lg);
  box-shadow: var(--shadow-md);
  backdrop-filter: blur(20px);
  animation: fadeIn var(--dur-slow) var(--ease-out);
  min-height: 0;
  max-height: 100%;
}

.message-history {
  flex: 1;
  min-height: 0;
  overflow-y: auto;
  padding: 16px;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

/* === User Message === */
.message-user { display: flex; justify-content: flex-end; }
.message-user .bubble {
  max-width: 75%;
  background: linear-gradient(135deg, var(--c-primary), var(--c-primary-dark));
  color: #fff;
  padding: 10px 16px;
  border-radius: 16px 16px 4px 16px;
  font-size: 14px;
  line-height: 1.5;
  animation: slideUp var(--dur-normal) var(--ease-out);
  box-shadow: 0 2px 12px rgba(108,92,231,0.25);
}
.message-user .sender-name {
  font-size: 11px;
  opacity: 0.75;
  margin-bottom: 3px;
  font-weight: 600;
  letter-spacing: 0.3px;
}

/* === Avatar Message === */
.message-avatar { display: flex; justify-content: flex-start; }
.message-avatar .bubble {
  max-width: 75%;
  background: var(--c-bg-card);
  border: 1px solid var(--c-border);
  color: var(--c-text);
  padding: 10px 16px;
  border-radius: 16px 16px 16px 4px;
  font-size: 14px;
  line-height: 1.5;
  animation: slideUp var(--dur-normal) var(--ease-out) 0.1s both;
}
.message-avatar .sender-name {
  font-size: 11px;
  color: var(--c-primary-light);
  margin-bottom: 3px;
  font-weight: 600;
}
.emotion-tags {
  margin-top: 6px;
  display: flex;
  gap: 4px;
  flex-wrap: wrap;
}
.emotion-tag {
  font-size: 11px;
  padding: 2px 10px;
  border-radius: 12px;
  background: rgba(108,92,231,0.15);
  color: var(--c-primary-light);
  border: 1px solid rgba(108,92,231,0.2);
}
.audio-status {
  margin-top: 6px;
  font-size: 12px;
  color: var(--c-text-muted);
  display: flex;
  align-items: center;
  gap: 6px;
}
.audio-indicator.playing { animation: pulse-glow 1s infinite; }
.timestamp {
  font-size: 10px;
  color: var(--c-text-muted);
  margin-top: 4px;
  text-align: right;
}

/* === System Message === */
.system-message {
  text-align: center;
  font-size: 12px;
  color: var(--c-text-muted);
  padding: 4px 0;
  animation: fadeIn var(--dur-normal) var(--ease-out);
}

/* === Loading === */
.loading-indicator {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 12px;
  color: var(--c-text-secondary);
  font-size: 13px;
}
.typing-dots { display: flex; gap: 3px; }
.typing-dots span {
  width: 6px; height: 6px;
  border-radius: 50%;
  background: var(--c-primary-light);
  animation: typingBounce 1.4s infinite;
}
.typing-dots span:nth-child(2) { animation-delay: 0.2s; }
.typing-dots span:nth-child(3) { animation-delay: 0.4s; }
@keyframes typingBounce {
  0%,60%,100% { opacity: 0.3; transform: translateY(0); }
  30% { opacity: 1; transform: translateY(-4px); }
}

/* === Input Section === */
.input-section {
  flex-shrink: 0;
  border-top: 1px solid var(--c-border);
  padding: 10px 14px;
  display: flex;
  flex-direction: column;
  gap: 6px;
  background: rgba(0,0,0,0.15);
}
.login-hint {
  padding: 10px;
  background: rgba(253,203,110,0.1);
  border: 1px dashed rgba(253,203,110,0.4);
  border-radius: var(--radius-sm);
  font-size: 13px;
  color: var(--c-warning);
  text-align: center;
  cursor: pointer;
  transition: all var(--dur-fast) var(--ease-out);
}
.login-hint:hover { background: rgba(253,203,110,0.15); border-color: var(--c-warning); }
.input-wrapper { flex: 1; display: flex; flex-direction: column; gap: 6px; }
.message-input {
  padding: 8px 14px;
  background: var(--c-bg-input);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  font-family: inherit;
  font-size: 14px;
  color: var(--c-text);
  resize: none;
  height: 44px;
  transition: border-color var(--dur-fast) var(--ease-out);
  line-height: 1.5;
}
.message-input::placeholder { color: var(--c-text-muted); }
.message-input:focus {
  outline: none;
  border-color: var(--c-border-focus);
  box-shadow: 0 0 0 3px rgba(108,92,231,0.1);
}
.message-input:disabled { opacity: 0.4; cursor: not-allowed; }
.char-counter {
  font-size: 11px;
  color: var(--c-text-muted);
  text-align: right;
}
.char-counter .warning { color: var(--c-warning); }
.char-counter .danger { color: var(--c-error); }

/* === Actions === */
.action-buttons { display: flex; gap: 6px; align-items: center; flex-shrink: 0; }
.emoji-picker-wrapper { position: relative; }
.emoji-toggle-btn {
  width: 34px; height: 34px;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-input);
  font-size: 16px;
  cursor: pointer;
  transition: all var(--dur-fast) var(--ease-out);
  display: flex;
  align-items: center;
  justify-content: center;
}
.emoji-toggle-btn:hover:not(:disabled) {
  border-color: var(--c-primary);
  background: rgba(108,92,231,0.1);
}
.emoji-toggle-btn.active { border-color: var(--c-primary); background: rgba(108,92,231,0.15); }
.emoji-toggle-btn:disabled { opacity: 0.4; cursor: not-allowed; }
.emoji-panel {
  position: absolute;
  bottom: 42px;
  left: 0;
  width: 304px;
  max-height: 320px;
  overflow-y: auto;
  background: var(--c-surface);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-md);
  box-shadow: var(--shadow-lg);
  padding: 10px;
  display: grid;
  grid-template-columns: repeat(8, 1fr);
  gap: 4px;
  z-index: 100;
  backdrop-filter: blur(20px);
  animation: fadeIn var(--dur-fast) var(--ease-out);
}
.emoji-item {
  width: 32px; height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 20px;
  cursor: pointer;
  border-radius: 6px;
  transition: all var(--dur-fast);
}
.emoji-item:hover { background: rgba(255,255,255,0.08); transform: scale(1.25); }
.send-button {
  padding: 8px 16px;
  background: linear-gradient(135deg, var(--c-primary), var(--c-primary-dark));
  color: #fff;
  border: none;
  border-radius: var(--radius-sm);
  cursor: pointer;
  font-weight: 600;
  font-size: 13px;
  white-space: nowrap;
  transition: all var(--dur-fast) var(--ease-out);
  letter-spacing: 0.3px;
  flex-shrink: 0;
}
.send-button:hover:not(:disabled) {
  transform: translateY(-1px);
  box-shadow: 0 4px 20px rgba(108,92,231,0.4);
}
.send-button:active:not(:disabled) { transform: translateY(0); }
.send-button:disabled { opacity: 0.4; cursor: not-allowed; }

/* === Connection === */
.connection-hint {
  flex-shrink: 0;
  padding: 4px 8px;
  text-align: center;
  font-size: 11px;
  min-height: 0;
}
.error-message { color: var(--c-error); }
.info-message { color: var(--c-text-muted); }

@media (max-width: 768px) {
  .message-history { padding: 10px; }
  .message-user .bubble, .message-avatar .bubble { max-width: 85%; }
  .input-section { padding: 8px 10px; gap: 6px; }
  .message-input { padding: 8px 12px; font-size: 13px; }
  .send-button { padding: 8px 14px; font-size: 13px; }
  .emoji-panel { width: 280px; grid-template-columns: repeat(7, 1fr); }
}
</style>
