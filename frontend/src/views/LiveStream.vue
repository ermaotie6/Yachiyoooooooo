<template>
  <div class="live-stream-container">
    <AvatarStage
      ref="avatarRef"
      :connection-status="connectionStatus"
      :subtitle-text="subtitle.subtitleText.value"
      @animation-complete="onAnimationComplete"
    />
    <ChatPanel
      ref="chatRef"
      :messages="chat.messages.value"
      :is-connected="isConnected"
      :is-processing="isProcessing"
      :connection-error="connectionError"
      :disabled="!isConnected || isProcessing"
      @send="sendMessage"
      @clear="handleClear"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { useWebSocket } from '@/composables/useWebSocket'
import { useAudioPlayer } from '@/composables/useAudioPlayer'
import { useChatMessages } from '@/composables/useChatMessages'
import { useSubtitle } from '@/composables/useSubtitle'
import { useAuthStore } from '@/stores/auth'
import AvatarStage from '@/components/live/AvatarStage.vue'
import ChatPanel from '@/components/live/ChatPanel.vue'

const authStore = useAuthStore()
const ws = useWebSocket()
const audio = useAudioPlayer()
const chat = useChatMessages()
const subtitle = useSubtitle()

const avatarRef = ref<InstanceType<typeof AvatarStage> | null>(null)
const chatRef = ref<InstanceType<typeof ChatPanel> | null>(null)

const isConnected = ref(false)
const isProcessing = ref(false)
const connectionError = ref<string | null>(null)

const stableGuestId = ref(`guest_${Date.now()}_${Math.random().toString(36).slice(2, 8)}`)
const currentUser = computed(() => ({
  name: authStore.user?.username || '匿名用户',
  id: authStore.user?.userId ? String(authStore.user.userId) : stableGuestId.value
}))

const connectionStatus = computed(() => isConnected.value ? 'connected' as const : 'disconnected' as const)

const sendMessage = async () => {
  const input = chatRef.value?.inputValue
  if (!input?.trim() || !isConnected.value) return

  const content = input.trim()
  chatRef.value!.clearInput()
  isProcessing.value = true

  ws.sendUserMessage(content, currentUser.value.id, currentUser.value.name)
}

const handleAvatarResponse = async (response: any) => {
  isProcessing.value = false

  chat.addAvatarResponse(response)

  // 字幕
  const fullText = response.text || ''
  const duration = response.audio_duration_ms || 0
  if (fullText && duration > 0) {
    subtitle.startProgressive(fullText, duration)
  } else {
    subtitle.setImmediate(fullText)
  }

  chat.scrollToBottom(chatRef.value?.historyRef || null)

  // 音频 + 动画
  if (response.audio_url) {
    const lastMsg = chat.latestAvatarMessage.value
    if (lastMsg) lastMsg.isAudioPlaying = true

    audio.onMouth(data => {
      avatarRef.value?.live2d?.setSyncMouthOpenY(data.mouthOpenY)
    })

    audio.onEnd(() => {
      if (lastMsg) lastMsg.isAudioPlaying = false
      subtitle.clear()
      avatarRef.value?.live2d?.setExpression('f_smile', 500)
    })

    await audio.play(response.audio_url)

    if (response.animation_commands) {
      playAnimations(response.animation_commands)
    }
  } else if (response.animation_commands) {
    playAnimations(response.animation_commands)
    setTimeout(() => {
      subtitle.clear()
      avatarRef.value?.live2d?.setExpression('f_smile', 500)
    }, 5000)
  }
}

const playAnimations = (commands: any[]) => {
  const live2d = avatarRef.value?.live2d
  if (!live2d) return

  for (const cmd of commands) {
    const params = cmd.params || cmd
    if (params.expression_name) {
      live2d.setExpression(params.expression_name, params.duration_ms || 1000)
    } else if (params.group) {
      live2d.playMotion(params.group, params.priority || 0)
    }
  }
}

const handleUserBroadcast = (broadcast: any) => {
  chat.addUserBroadcast(broadcast)
  chat.scrollToBottom(chatRef.value?.historyRef || null)
}

const handleConnectionChange = (connected: boolean) => {
  isConnected.value = connected
  connectionError.value = null
  chat.addSystemMessage(connected ? '✅ 已连接到服务器' : '❌ 连接已断开')
}

const handleError = (error: string) => {
  isProcessing.value = false
  if (error.toLowerCase().includes('connection') || error.toLowerCase().includes('websocket')) {
    connectionError.value = error
  }
  chat.addSystemMessage(`⚠️ ${error}`)
}

const handleClear = () => {
  if (confirm('确定要清空聊天历史吗？')) {
    chat.clearMessages()
    chat.addSystemMessage('聊天历史已清空')
  }
}

const onAnimationComplete = () => console.log('动画播放完成')

onMounted(async () => {
  ws.onAvatar(handleAvatarResponse)
  ws.onErr(handleError)
  ws.onConnectionStatusChange(handleConnectionChange)
  ws.onBroadcast(handleUserBroadcast)

  try {
    await ws.connect(currentUser.value.id)
  } catch (e) {
    connectionError.value = String(e)
    chat.addSystemMessage(`❌ 连接失败: ${e}`)
  }
})

onUnmounted(async () => {
  subtitle.clearTimer()
  await ws.disconnect()
  audio.cleanup()
})
</script>

<style scoped>
.live-stream-container {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 20px;
  padding: 20px;
  height: 100vh;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  background-image: url('/images/bg.jpg'), url('/images/bg.svg');
  background-size: cover;
  background-position: center;
  background-repeat: no-repeat;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
}

@media (max-width: 1200px) {
  .live-stream-container {
    grid-template-columns: 1fr;
    height: auto;
  }
}

@media (max-width: 768px) {
  .live-stream-container {
    padding: 10px;
    gap: 10px;
  }
}
</style>
