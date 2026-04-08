<template>
  <div class="live-stream-container">
    <!-- 虚拟形象区域 -->
    <section class="avatar-section">
      <div class="avatar-wrapper">
        <Live2DComponent
          ref="live2dComponent"
          :width="800"
          :height="600"
          @animation-complete="onAnimationComplete"
        />
      </div>

      <!-- 连接状态指示器 -->
      <div class="status-indicator">
        <span :class="['status-dot', connectionStatus]"></span>
        <span class="status-text">
          {{ connectionStatus === 'connected' ? '已连接' : '已断开' }}
        </span>
      </div>

      <!-- 虚拟形象信息 -->
      <div class="avatar-info">
        <h2>Yachiyo</h2>
        <p class="role">AI 虚拟直播助手</p>
      </div>

      <!-- 字幕覆盖层 -->
      <div v-if="subtitleText" class="subtitle-overlay">
        <span class="subtitle-text">{{ subtitleText }}</span>
      </div>
    </section>

    <!-- 聊天区域 -->
    <section class="chat-section">
      <!-- 消息历史 -->
      <div class="message-history" ref="messageHistoryRef">
        <div
          v-for="msg in messages"
          :key="msg.id"
          :class="['message', `message-${msg.role}`]"
        >
          <!-- 用户消息 -->
          <div v-if="msg.role === 'user'" class="user-message">
            <div class="sender-name">{{ msg.senderName || currentUser.name }}</div>
            <div class="content">{{ msg.text }}</div>
            <div class="timestamp">{{ formatTime(msg.timestamp) }}</div>
          </div>

          <!-- Avatar 响应 -->
          <div v-if="msg.role === 'avatar'" class="avatar-message">
            <div class="sender-name">Yachiyo</div>
            <div class="content">{{ msg.text }}</div>

            <!-- 情感标签 -->
            <div v-if="msg.emotions && msg.emotions.length > 0" class="emotion-tags">
              <span v-for="emotion in msg.emotions" :key="emotion" class="emotion-tag">
                {{ emotion }}
              </span>
            </div>

            <!-- 音频播放状态 -->
            <div v-if="msg.audioUrl" class="audio-status">
              <span class="audio-indicator" :class="{ playing: msg.isAudioPlaying }">
                🔊
              </span>
              <span class="duration">
                {{ formatDuration(msg.audioDuration) }}
              </span>
            </div>

            <div class="timestamp">{{ formatTime(msg.timestamp) }}</div>
          </div>

          <!-- 系统消息 -->
          <div v-if="msg.role === 'system'" class="system-message">
            {{ msg.text }}
          </div>
        </div>

        <!-- 加载状态 -->
        <div v-if="isProcessing" class="loading-indicator">
          <div class="spinner"></div>
          <span>处理中...</span>
        </div>
      </div>

      <!-- 消息输入区域 -->
      <div class="input-section">
        <div class="input-wrapper">
          <textarea
            v-model="messageInput"
            placeholder="输入你的消息（最多50字）..."
            :disabled="!isConnected || isProcessing"
            @keydown.enter.ctrl="sendMessage"
            @input="onInputLimit"
            class="message-input"
          />

          <!-- 字数统计 -->
          <div class="char-counter">
            <span :class="{ warning: charCount > 40, danger: charCount > 45 }">
              {{ charCount }}/50
            </span>
          </div>
        </div>

        <!-- 操作按钮 -->
        <div class="action-buttons">
          <!-- Emoji 选择器 -->
          <div class="emoji-picker-wrapper">
            <button
              @click="toggleEmojiPicker"
              class="emoji-toggle-btn"
              :class="{ active: showEmojiPicker }"
              title="表情"
              :disabled="!isConnected || isProcessing"
            >
              😊
            </button>
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
            @click="sendMessage"
            :disabled="!isConnected || isProcessing || !messageInput.trim()"
            class="send-button"
          >
            <span v-if="!isProcessing">发送</span>
            <span v-else>
              <span class="button-spinner"></span>
              处理中
            </span>
          </button>

          <button
            @click="clearHistory"
            :disabled="messages.length === 0"
            class="clear-button"
            title="清空聊天历史"
          >
            清空
          </button>
        </div>
      </div>

      <!-- 连接提示 -->
      <div v-if="!isConnected" class="connection-hint">
        <span v-if="connectionError" class="error-message">
          ❌ {{ connectionError }}
        </span>
        <span v-else class="info-message">
          ⏳ 尝试连接中...
        </span>
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, nextTick } from 'vue'
import { useWebSocket } from '@/composables/useWebSocket'
import { useAudioPlayer } from '@/composables/useAudioPlayer'
import { useAuthStore } from '@/stores/auth'
import Live2DComponent from '@/components/Live2DComponent.vue'

// ============ 状态管理 ============

interface Message {
  id: string
  role: 'user' | 'avatar' | 'system'
  text: string
  senderName?: string
  emotions?: string[]
  actions?: string[]
  audioUrl?: string
  audioDuration?: number
  isAudioPlaying?: boolean
  timestamp: number
}

const messageInput = ref('')
const messages = ref<Message[]>([])
const isConnected = ref(false)
const isProcessing = ref(false)
const connectionError = ref<string | null>(null)
const subtitleText = ref('')

// Emoji 选择器
const showEmojiPicker = ref(false)
const emojiList = [
  '😊', '😂', '🤣', '❤️', '👍', '👋', '🎉', '🌸',
  '😍', '🤔', '😢', '😡', '😱', '😳', '🤩', '🥺',
  '💛', '💚', '💙', '🖤', '🔥', '✨', '🌟', '🌞',
  '🐶', '🐱', '🍀', '🍒', '🍰', '☕', '🌺', '🌻'
]

const toggleEmojiPicker = () => {
  showEmojiPicker.value = !showEmojiPicker.value
}

const insertEmoji = (emoji: string) => {
  // 使用 Array.from 正确统计 Unicode 字符数（包括 emoji 组合符）
  if (Array.from(messageInput.value).length < 50) {
    messageInput.value += emoji
  }
  showEmojiPicker.value = false
}

/**
 * 输入限制 — 用 Unicode 字符计数截断超长输入
 * 替代原生 maxlength（原生按 UTF-16 代码单元计数，对 emoji 不准确）
 */
const onInputLimit = () => {
  const chars = Array.from(messageInput.value)
  if (chars.length > 50) {
    messageInput.value = chars.slice(0, 50).join('')
  }
}

// 字幕渐进显示
let subtitleTimer: ReturnType<typeof setTimeout> | null = null

const authStore = useAuthStore()
// 稳定的 guest ID — 使用 ref 保证在整个组件生命周期内不变
const stableGuestId = ref(`guest_${Date.now()}_${Math.random().toString(36).slice(2, 8)}`)
const currentUser = computed(() => ({
  name: authStore.user?.username || '匿名用户',
  id: authStore.user?.userId ? String(authStore.user.userId) : stableGuestId.value
}))

const connectionStatus = computed(() =>
  isConnected.value ? 'connected' : 'disconnected'
)

// Unicode 字符计数 — Array.from 正确处理 emoji 等多字节字符
const charCount = computed(() => Array.from(messageInput.value).length)

// ============ 组件引用 ============

const messageHistoryRef = ref<HTMLElement | null>(null)
const live2dComponent = ref<InstanceType<typeof Live2DComponent> | null>(null)

const ws = useWebSocket()
const audioPlayer = useAudioPlayer()

// ============ 事件处理 ============

/**
 * 发送消息
 */
const sendMessage = async () => {
  if (!messageInput.value.trim()) return
  if (!isConnected.value) return

  const content = messageInput.value.trim()

  // 添加用户消息到历史
  const now = Date.now()
  messages.value.push({
    id: `msg_${now}_${Math.random().toString(36).slice(2, 8)}`,
    role: 'user',
    text: content,
    timestamp: now
  })

  messageInput.value = ''
  isProcessing.value = true

  // 滚动到底部
  await nextTick()
  scrollToBottom()

  try {
    // 发送给后端（携带 userId 和 username 供广播使用）
    const sent = ws.sendUserMessage(content, currentUser.value.id, currentUser.value.name)
    if (!sent) {
      // 消息被排队（WebSocket 未连接），不会收到 avatar_response，需重置状态
      isProcessing.value = false
      addSystemMessage('⚠️ 消息已排队，等待连接恢复后自动发送')
    }
  } catch (error) {
    console.error('发送消息失败:', error)
    isProcessing.value = false
    addSystemMessage('发送消息失败，请重试')
  }
}

/**
 * 处理 Avatar 响应
 */
const handleAvatarResponse = async (response: any) => {
  isProcessing.value = false

  // 添加 Avatar 消息
  const avatarMsg: Message = {
    id: `msg_${Date.now()}`,
    role: 'avatar',
    text: response.text,
    emotions: response.emotions || [],
    actions: response.actions || [],
    audioUrl: response.audio_url,
    audioDuration: response.audio_duration_ms,
    timestamp: Date.now()
  }

  messages.value.push(avatarMsg)

  // 显示字幕（渐进式，根据音频时长同步刻字）
  const fullText = response.text || ''
  const audioDuration = response.audio_duration_ms || 0
  if (fullText && audioDuration > 0) {
    startProgressiveSubtitle(fullText, audioDuration)
  } else {
    subtitleText.value = fullText
  }

  // 滚动到底部
  await nextTick()
  scrollToBottom()

  // 播放音频（如果有）
  if (response.audio_url) {
    try {
      await playAudioWithAnimations(response)
    } catch (error) {
      console.error('播放音频失败:', error)
    }
  } else if (response.animation_commands && response.animation_commands.length > 0) {
    // 只有动画，没有音频
    playAnimations(response.animation_commands)
  }

  // 如果没有音频，延迟清除字幕
  if (!response.audio_url) {
    setTimeout(() => {
      subtitleText.value = ''
    }, 5000)
  }
}

/**
 * 播放音频和动画
 */
const playAudioWithAnimations = async (response: any) => {
  // 标记音频正在播放
  const lastMessage = messages.value[messages.value.length - 1]
  if (lastMessage) {
    lastMessage.isAudioPlaying = true
  }

  try {
    // 启动音频分析用于嘴部同步
    audioPlayer.onMouth((data) => {
      if (live2dComponent.value) {
        live2dComponent.value.setSyncMouthOpenY(data.mouthOpenY)
      }
    })

    // 先注册播放结束回调（在 play 之前），防止极短音频在回调注册前就播完导致 Promise 永不 resolve
    const playEndPromise = new Promise<void>((resolve) => {
      audioPlayer.onEnd(() => {
        if (lastMessage) {
          lastMessage.isAudioPlaying = false
        }
        // 音频播放结束后清除字幕
        clearSubtitleTimer()
        subtitleText.value = ''
        resolve()
      })
    })

    // 播放音频
    await audioPlayer.play(response.audio_url)

    // 播放动画
    if (response.animation_commands) {
      playAnimations(response.animation_commands)
    }

    // 等待音频播放完成
    await playEndPromise
  } catch (error) {
    console.error('音频播放错误:', error)
    if (lastMessage) {
      lastMessage.isAudioPlaying = false
    }
    clearSubtitleTimer()
    subtitleText.value = ''
  }
}

/**
 * 播放动画序列
 * 后端发送的动画命令格式: { type: "expression"|"motion"|"parameter", params: {...} }
 */
const playAnimations = (commands: any[]) => {
  if (!live2dComponent.value) return

  for (const cmd of commands) {
    const cmdType = cmd.type || ''
    const params = cmd.params || cmd

    if (cmdType === 'expression' || params.expression_name) {
      // 表情命令: { type: "expression", params: { expression_name: "f_smile", duration_ms: 2000 } }
      const exprName = params.expression_name || params.expression || ''
      if (exprName) {
        live2dComponent.value.setExpression(
          exprName,
          params.duration_ms || 1000
        )
      }
    } else if (cmdType === 'motion' || params.group) {
      // 动作命令: { type: "motion", params: { group: "Tap Body", index: 0, priority: 1 } }
      const motionGroup = params.group || params.motion || ''
      if (motionGroup) {
        live2dComponent.value.playMotion(motionGroup, params.priority || 0)
      }
    }
    // parameter 类型 (嘴部同步) 由前端 Web Audio API 实时驱动，此处忽略
  }
}

/**
 * 处理其他用户的广播消息（实时消息框）
 */
const handleUserBroadcast = (broadcast: { sender_id: string; sender_name: string; content: string; timestamp: number }) => {
  // 跳过自己发送的消息（已在 sendMessage 中添加到本地）
  // 显式 String() 确保类型一致性比较
  if (String(broadcast.sender_id) === String(currentUser.value.id)) return

  messages.value.push({
    id: `msg_${broadcast.timestamp}_${Math.random().toString(36).slice(2, 8)}`,
    role: 'user',
    text: broadcast.content,
    timestamp: broadcast.timestamp,
    senderName: broadcast.sender_name
  })

  nextTick(() => scrollToBottom())
}

/**
 * 处理连接状态变化
 */
const handleConnectionStatusChange = (connected: boolean) => {
  isConnected.value = connected
  connectionError.value = null

  if (connected) {
    addSystemMessage('✅ 已连接到服务器')
  } else {
    addSystemMessage('❌ 连接已断开')
  }
}

/**
 * 处理错误
 */
const handleError = (error: string) => {
  console.error('错误:', error)
  connectionError.value = error
  // 收到后端错误消息，说明本次请求处理失败，重置处理状态防止 UI 锁死
  isProcessing.value = false
  addSystemMessage(`⚠️ 错误: ${error}`)
}

/**
 * 处理状态更新
 */
const handleStatusUpdate = (status: any) => {
  if (status.status === 'processing') {
    addSystemMessage(`处理中 (${status.progress}%)...`)
  } else if (status.status === 'completed') {
    // Avatar 响应会通过 onAvatarResponse 处理
  } else if (status.status === 'error') {
    addSystemMessage(`❌ 处理失败: ${status.message}`)
    isProcessing.value = false
  }
}

/**
 * 添加系统消息
 */
const addSystemMessage = (text: string) => {
  messages.value.push({
    id: `msg_${Date.now()}`,
    role: 'system',
    text,
    timestamp: Date.now()
  })
}

/**
 * 渐进式字幕显示：根据音频时长逐字刻出字幕
 * @param fullText 完整字幕文本
 * @param durationMs 音频时长（毫秒）
 */
const startProgressiveSubtitle = (fullText: string, durationMs: number) => {
  clearSubtitleTimer()

  const chars = Array.from(fullText)  // 支持 Unicode / emoji
  const totalChars = chars.length
  if (totalChars === 0) return

  // 每个字符的间隔时间，留出 10% 尾部时间保持全文显示
  const charInterval = Math.max(50, (durationMs * 0.9) / totalChars)
  let currentIndex = 0

  subtitleText.value = chars[0]  // 立刻显示第一个字
  currentIndex = 1

  const tick = () => {
    if (currentIndex < totalChars) {
      subtitleText.value = chars.slice(0, currentIndex + 1).join('')
      currentIndex++
      subtitleTimer = setTimeout(tick, charInterval)
    }
    // 全部显示完毕后等待 onEnd 回调清除
  }

  subtitleTimer = setTimeout(tick, charInterval)
}

/**
 * 清除字幕定时器
 */
const clearSubtitleTimer = () => {
  if (subtitleTimer !== null) {
    clearTimeout(subtitleTimer)
    subtitleTimer = null
  }
}

/**
 * 清空聊天历史
 */
const clearHistory = () => {
  if (confirm('确定要清空聊天历史吗？')) {
    messages.value = []
    addSystemMessage('聊天历史已清空')
  }
}

/**
 * 动画播放完成回调
 */
const onAnimationComplete = () => {
  console.log('动画播放完成')
}

// 字数统计由模板中 messageInput.length 直接驱动，无需额外方法

// ============ 工具方法 ============

/**
 * 格式化时间
 */
const formatTime = (timestamp: number): string => {
  const date = new Date(timestamp)
  const hours = String(date.getHours()).padStart(2, '0')
  const minutes = String(date.getMinutes()).padStart(2, '0')
  return `${hours}:${minutes}`
}

/**
 * 格式化音频时长
 */
const formatDuration = (ms?: number): string => {
  if (!ms || isNaN(ms) || ms <= 0) return '0:00'
  const seconds = Math.floor(ms / 1000)
  const minutes = Math.floor(seconds / 60)
  const secs = seconds % 60
  return `${minutes}:${String(secs).padStart(2, '0')}`
}

/**
 * 滚动到底部
 */
const scrollToBottom = () => {
  if (messageHistoryRef.value) {
    messageHistoryRef.value.scrollTop = messageHistoryRef.value.scrollHeight
  }
}

// ============ 生命周期 ============

onMounted(async () => {
  try {
    // 先注册事件监听器，防止连接成功后立即收到消息而丢失
    ws.onAvatar(handleAvatarResponse)
    ws.onStatus(handleStatusUpdate)
    ws.onErr(handleError)
    ws.onConnectionStatusChange(handleConnectionStatusChange)
    ws.onBroadcast(handleUserBroadcast)

    // 再连接到 WebSocket
    // 注意: 连接成功/失败的消息由 handleConnectionStatusChange 回调统一处理
    await ws.connect(currentUser.value.id)
  } catch (error) {
    console.error('连接失败:', error)
    connectionError.value = String(error)
    // 仅在回调未触发的情况下补充错误消息（如 connect Promise 直接 reject）
    if (!messages.value.some((m: Message) => m.text.includes('连接失败') || m.text.includes('连接已断开'))) {
      addSystemMessage(`❌ 连接失败: ${error}`)
    }
  }
})

onUnmounted(async () => {
  clearSubtitleTimer()
  await ws.disconnect()
  audioPlayer.cleanup()
})
</script>

<style scoped>
.live-stream-container {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 20px;
  padding: 20px;
  height: 100vh;
  /* 背景图片可替换：将 bg.jpg 放入 public/images/ 目录即可覆盖默认背景 */
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  background-image: url('/images/bg.jpg'), url('/images/bg.svg');
  background-size: cover;
  background-position: center;
  background-repeat: no-repeat;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen,
    Ubuntu, Cantarell, sans-serif;
}

/* ============ 虚拟形象区域 ============ */

.avatar-section {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background: rgba(255, 255, 255, 0.1);
  border-radius: 15px;
  backdrop-filter: blur(10px);
  position: relative;
  overflow: hidden;
}

.avatar-wrapper {
  flex: 1;
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  max-height: 80%;
}

.status-indicator {
  position: absolute;
  top: 20px;
  right: 20px;
  display: flex;
  align-items: center;
  gap: 10px;
  color: white;
  font-size: 14px;
  font-weight: 500;
  background: rgba(0, 0, 0, 0.3);
  padding: 8px 12px;
  border-radius: 20px;
  backdrop-filter: blur(5px);
}

.status-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  animation: pulse 2s infinite;
}

.status-dot.connected {
  background-color: #4caf50;
}

.status-dot.disconnected {
  background-color: #f44336;
}

@keyframes pulse {
  0%,
  100% {
    opacity: 1;
  }
  50% {
    opacity: 0.5;
  }
}

.avatar-info {
  position: absolute;
  bottom: 20px;
  left: 20px;
  color: white;
  z-index: 10;
}

.avatar-info h2 {
  margin: 0;
  font-size: 24px;
  font-weight: bold;
}

.avatar-info .role {
  margin: 5px 0 0 0;
  font-size: 12px;
  opacity: 0.8;
}

/* ============ 字幕覆盖层 ============ */

.subtitle-overlay {
  position: absolute;
  bottom: 60px;
  left: 50%;
  transform: translateX(-50%);
  z-index: 20;
  max-width: 90%;
  pointer-events: none;
  animation: subtitleFadeIn 0.3s ease-out;
}

.subtitle-text {
  display: inline-block;
  background: rgba(0, 0, 0, 0.7);
  color: #fff;
  font-size: 18px;
  line-height: 1.6;
  padding: 10px 24px;
  border-radius: 8px;
  text-align: center;
  word-break: break-word;
  backdrop-filter: blur(4px);
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.5);
}

@keyframes subtitleFadeIn {
  from {
    opacity: 0;
    transform: translateX(-50%) translateY(8px);
  }
  to {
    opacity: 1;
    transform: translateX(-50%) translateY(0);
  }
}

/* ============ 聊天区域 ============ */

.chat-section {
  display: flex;
  flex-direction: column;
  background: rgba(255, 255, 255, 0.95);
  border-radius: 15px;
  overflow: hidden;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
}

.message-history {
  flex: 1;
  overflow-y: auto;
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 15px;
  scroll-behavior: smooth;
}

.message-history::-webkit-scrollbar {
  width: 8px;
}

.message-history::-webkit-scrollbar-track {
  background: transparent;
}

.message-history::-webkit-scrollbar-thumb {
  background: rgba(0, 0, 0, 0.2);
  border-radius: 4px;
}

.message-history::-webkit-scrollbar-thumb:hover {
  background: rgba(0, 0, 0, 0.3);
}

.message {
  display: flex;
  animation: slideIn 0.3s ease-out;
}

@keyframes slideIn {
  from {
    opacity: 0;
    transform: translateY(10px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.message-user {
  justify-content: flex-end;
}

.message-avatar {
  justify-content: flex-start;
}

.message-system {
  justify-content: center;
}

.user-message {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  padding: 12px 16px;
  border-radius: 15px;
  max-width: 75%;
  word-wrap: break-word;
  box-shadow: 0 2px 8px rgba(102, 126, 234, 0.3);
}

.avatar-message {
  background: #f5f5f5;
  color: #333;
  padding: 12px 16px;
  border-radius: 15px;
  max-width: 75%;
  border-left: 3px solid #667eea;
}

.system-message {
  background: rgba(0, 0, 0, 0.05);
  color: #999;
  padding: 8px 12px;
  border-radius: 8px;
  font-size: 12px;
  text-align: center;
  max-width: 80%;
}

.sender-name {
  font-weight: bold;
  font-size: 12px;
  margin-bottom: 5px;
  opacity: 0.7;
}

.content {
  line-height: 1.5;
  word-break: break-word;
}

.emotion-tags {
  display: flex;
  gap: 5px;
  margin-top: 8px;
  flex-wrap: wrap;
}

.emotion-tag {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  padding: 3px 8px;
  border-radius: 12px;
  font-size: 10px;
  font-weight: 500;
}

.audio-status {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-top: 8px;
  font-size: 12px;
}

.audio-indicator {
  font-size: 14px;
  animation: none;
}

.audio-indicator.playing {
  animation: bounce 1s infinite;
}

@keyframes bounce {
  0%,
  100% {
    transform: scale(1);
  }
  50% {
    transform: scale(1.2);
  }
}

.duration {
  color: #999;
  font-size: 11px;
}

.timestamp {
  font-size: 11px;
  opacity: 0.5;
  margin-top: 5px;
}

.loading-indicator {
  display: flex;
  align-items: center;
  gap: 10px;
  justify-content: center;
  color: #999;
  font-size: 12px;
}

.spinner {
  width: 16px;
  height: 16px;
  border: 2px solid #f3f3f3;
  border-top: 2px solid #667eea;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  0% {
    transform: rotate(0deg);
  }
  100% {
    transform: rotate(360deg);
  }
}

/* ============ 输入区域 ============ */

.input-section {
  padding: 20px;
  border-top: 1px solid #eee;
  display: flex;
  gap: 10px;
  background: white;
}

.input-wrapper {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.message-input {
  flex: 1;
  padding: 12px;
  border: 2px solid #e0e0e0;
  border-radius: 8px;
  font-family: inherit;
  font-size: 14px;
  resize: vertical;
  max-height: 100px;
  min-height: 40px;
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

.char-counter span.warning {
  color: #ff9800;
  font-weight: bold;
}

.char-counter span.danger {
  color: #f44336;
  font-weight: bold;
}

.action-buttons {
  display: flex;
  gap: 10px;
  align-items: flex-end;
}

/* ============ Emoji 选择器 ============ */

.emoji-picker-wrapper {
  position: relative;
}

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

.emoji-toggle-btn:hover:not(:disabled) {
  border-color: #667eea;
  background: #f5f3ff;
}

.emoji-toggle-btn.active {
  border-color: #667eea;
  background: #ede9fe;
}

.emoji-toggle-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.emoji-panel {
  position: absolute;
  bottom: 50px;
  right: 0;
  width: 280px;
  max-height: 200px;
  overflow-y: auto;
  background: white;
  border: 1px solid #e0e0e0;
  border-radius: 12px;
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.15);
  padding: 10px;
  display: grid;
  grid-template-columns: repeat(8, 1fr);
  gap: 4px;
  z-index: 100;
  animation: emojiSlideIn 0.2s ease-out;
}

@keyframes emojiSlideIn {
  from {
    opacity: 0;
    transform: translateY(8px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.emoji-item {
  width: 32px;
  height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 20px;
  cursor: pointer;
  border-radius: 6px;
  transition: background 0.15s;
}

.emoji-item:hover {
  background: #f0f0f0;
  transform: scale(1.15);
}

.send-button,
.clear-button {
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

.send-button:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
}

.send-button:active:not(:disabled) {
  transform: translateY(0);
}

.send-button:disabled,
.clear-button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.clear-button {
  background: linear-gradient(135deg, #e0e0e0 0%, #bdbdbd 100%);
  color: #666;
  padding: 10px 16px;
}

.button-spinner {
  display: inline-block;
  width: 14px;
  height: 14px;
  border: 2px solid rgba(255, 255, 255, 0.3);
  border-top-color: white;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

.connection-hint {
  padding: 10px;
  text-align: center;
  font-size: 12px;
  background: #fff9c4;
  border-top: 1px solid #eee;
  color: #f57f17;
}

.error-message {
  color: #d32f2f;
}

.info-message {
  color: #f57f17;
}

/* ============ 响应式设计 ============ */

@media (max-width: 1200px) {
  .live-stream-container {
    grid-template-columns: 1fr;
    height: auto;
  }

  .avatar-section {
    min-height: 400px;
  }

  .chat-section {
    min-height: 400px;
  }
}

@media (max-width: 768px) {
  .live-stream-container {
    padding: 10px;
    gap: 10px;
  }

  .user-message,
  .avatar-message {
    max-width: 90%;
  }

  .input-section {
    padding: 10px;
    gap: 5px;
  }

  .message-input {
    padding: 8px;
    font-size: 13px;
  }

  .send-button {
    padding: 8px 16px;
    font-size: 12px;
  }
}
</style>
