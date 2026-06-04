<template>
  <div class="live-stream-container" @click.once="unlockAudio">
    <section class="avatar-section">
      <Live2DComponent
        ref="live2dComponent"
        :width="800"
        :height="600"
        @animation-complete="onAnimationComplete"
      />
      <!-- 连接状态 -->
      <div class="status-indicator">
        <span :class="['status-dot', connectionStatus]"></span>
        <span class="status-text">{{ connectionStatus === 'connected' ? '已连接' : '已断开' }}</span>
      </div>
      <!-- 字幕 -->
      <div v-if="subtitle.subtitleText.value" class="subtitle-overlay">
        <span class="subtitle-text">{{ subtitle.subtitleText.value }}</span>
      </div>
      <!-- 半透明对话框覆盖 -->
      <ChatPanel
        ref="chatRef"
        :messages="chat.messages.value"
        :is-connected="isConnected"
        :is-processing="isProcessing"
        :connection-error="connectionError"
        :is-logged-in="authStore.isLoggedIn"
        :disabled="!isConnected || isProcessing"
        @send="handleSendOrLogin"
        @login-required="authDialogVisible = true"
      />
    </section>
    <AuthDialog v-model:visible="authDialogVisible" />
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { useWebSocket } from '@/composables/useWebSocket'
import { useAudioPlayer } from '@/composables/useAudioPlayer'
import { useChatMessages } from '@/composables/useChatMessages'
import { useSubtitle } from '@/composables/useSubtitle'
import { useAuthStore } from '@/stores/auth'
import { api } from '@/api/client'
import ChatPanel from '@/components/live/ChatPanel.vue'
import AuthDialog from '@/components/AuthDialog.vue'
import Live2DComponent from '@/components/Live2DComponent.vue'

const authStore = useAuthStore()
const ws = useWebSocket()
const audio = useAudioPlayer()
const chat = useChatMessages()
const subtitle = useSubtitle()

const avatarRef = ref<InstanceType<typeof Live2DComponent> | null>(null)
const chatRef = ref<InstanceType<typeof ChatPanel> | null>(null)
const authDialogVisible = ref(false)

const isConnected = ref(false)
const isProcessing = ref(false)
const connectionError = ref<string | null>(null)
let audioUnlocked = false

const stableGuestId = ref(`guest_${Date.now()}_${Math.random().toString(36).slice(2, 8)}`)
const currentUser = computed(() => ({
  name: authStore.user?.username || '匿名用户',
  id: authStore.user?.userId ? String(authStore.user.userId) : stableGuestId.value
}))

const connectionStatus = computed(() => isConnected.value ? 'connected' as const : 'disconnected' as const)

const handleSendOrLogin = () => {
  if (!authStore.isLoggedIn) {
    authDialogVisible.value = true
    return
  }
  // 解锁浏览器的音频自动播放策略（需要用户手势）
  unlockAudio()
  sendMessage()
}

const unlockAudio = () => {
  if (audioUnlocked) return
  // 同时解锁 AudioContext 和 SpeechSynthesis
  try {
    const ac = new AudioContext()
    const buf = ac.createBuffer(1, 1, 22050)
    const src = ac.createBufferSource()
    src.buffer = buf; src.connect(ac.destination)
    src.start(0)
    src.onended = () => ac.close()
  } catch { /* ignore */ }
  // SpeechSynthesis 预热
  const synth = window.speechSynthesis
  if (synth) {
    synth.cancel()
    // Chrome 需要第一声在实际用户手势中触发，后续允许
    const warm = new SpeechSynthesisUtterance('')
    warm.volume = 0; warm.rate = 0.01
    synth.speak(warm)
  }
  audioUnlocked = true
  console.log('[Audio] Unlocked (AudioContext + SpeechSynthesis)')
}

const sendMessage = async () => {
  const input = chatRef.value?.inputValue
  if (!input?.trim() || !isConnected.value) return

  const content = input.trim()
  // 提示词注入防护：拦截常见注入模式
  const injectionPatterns = [
    /ignore\s+(all\s+)?(previous|prior|above|system)\s+(instructions?|prompts?|messages?)/i,
    /you\s+are\s+now\s+(a|an|the)\s+new/i,
    /forget\s+(everything|all)/i,
    /system\s*:\s*|\[system\]|\<system\>/i,
    /override\s+(system\s+)?prompt/i,
    /act\s+as\s+(if\s+you\s+are|a\s+different)/i,
    /disregard\s+(all\s+)?(previous|prior|above)/i,
    /do\s+not\s+follow\s+(your|the)\s+(instructions?|rules?)/i,
  ]
  for (const pattern of injectionPatterns) {
    if (pattern.test(content)) {
      chat.addSystemMessage('⚠️ 消息包含不安全内容，已拦截')
      chatRef.value!.clearInput()
      return
    }
  }
  // 长度限制
  if (content.length > 500) {
    chat.addSystemMessage('⚠️ 消息过长，最多500字')
    return
  }

  // 立即将用户消息加入本地数组（避免等待 WebSocket 广播回传）
  chat.addMessage({
    id: `msg_${Date.now()}_${Math.random().toString(36).slice(2, 8)}`,
    role: 'user',
    text: content,
    senderName: currentUser.value.name,
    timestamp: Date.now()
  })
  // 同时加入去重哈希，防止 WebSocket 广播回传时重复
  const hash = `${currentUser.value.name}|${content}`
  sentMessageHashes.add(hash)
  chat.scrollToBottom(chatRef.value?.historyRef || null)

  chatRef.value!.clearInput()
  isProcessing.value = true

  ws.sendUserMessage(content, currentUser.value.id, currentUser.value.name)

  // 超时保护：30 秒后若仍无响应则恢复发送按钮
  const timeoutHandle = setTimeout(() => {
    if (isProcessing.value) {
      isProcessing.value = false
      chat.addSystemMessage('⏰ 响应超时，请重试')
      chat.scrollToBottom(chatRef.value?.historyRef || null)
    }
  }, 30_000)
  // 在 handleAvatarResponse 和 handleError 中清除超时
  ;(window as any).__avatarTimeout = timeoutHandle
}

const handleAvatarResponse = async (response: any) => {
  // 清除超时定时器
  if ((window as any).__avatarTimeout) {
    clearTimeout((window as any).__avatarTimeout)
    ;(window as any).__avatarTimeout = null
  }
  isProcessing.value = false

  chat.addAvatarResponse(response)

  // 字幕 — 优先显示中文原文，音频输出是日语
  const subtitleText = response.original_text || response.text || ''
  const duration = response.audio_duration_ms || 0
  if (subtitleText && duration > 0) {
    subtitle.startProgressive(subtitleText, duration)
  } else {
    subtitle.setImmediate(subtitleText)
  }

  chat.scrollToBottom(chatRef.value?.historyRef || null)

  // 音频 + 动画
  if (response.audio_url) {
    const lastMsg = chat.latestAvatarMessage.value
    if (lastMsg) lastMsg.isAudioPlaying = true

    audio.onMouth(data => {
      avatarRef.value?.setSyncMouthOpenY(data.mouthOpenY)
    })

    audio.onEnd(() => {
      if (lastMsg) lastMsg.isAudioPlaying = false
      subtitle.clear()
      avatarRef.value?.setExpression('f_smile', 500)
    })

    await audio.play(response.audio_url)

    if (response.animation_commands) {
      playAnimations(response.animation_commands)
    }
  } else if (response.animation_commands) {
    playAnimations(response.animation_commands)
    setTimeout(() => {
      subtitle.clear()
      avatarRef.value?.setExpression('f_smile', 500)
    }, 5000)
  }

  // 浏览器 TTS fallback（mock 模式或 GPT-SoVITS 不可用时）
  if (!response.audio_url && response.text) {
    const speakFallback = () => {
      try {
        const synth = window.speechSynthesis
        if (!synth) {
          chat.addSystemMessage('🔇 浏览器不支持语音合成')
          return
        }
        synth.cancel()

        const ttsText = response.original_text || response.text
        const utterance = new SpeechSynthesisUtterance(ttsText)

        // 确保 voices 已加载（Chrome 异步加载）
        let voices = synth.getVoices()
        if (voices.length === 0) {
          // 同步获取可能为空，尝试等待
          setTimeout(() => {
            const v2 = synth.getVoices()
            const zh = v2.find((v: SpeechSynthesisVoice) => v.lang.startsWith('zh-CN'))
              || v2.find((v: SpeechSynthesisVoice) => v.lang.startsWith('zh'))
            if (zh) { utterance.voice = zh; utterance.lang = zh.lang }
            else utterance.lang = 'zh-CN'
            utterance.rate = 1.0; utterance.pitch = 1.0; utterance.volume = 1.0
            synth.speak(utterance)
          }, 100)
          return
        }

        const zhVoice = voices.find((v: SpeechSynthesisVoice) => v.lang.startsWith('zh-CN'))
          || voices.find((v: SpeechSynthesisVoice) => v.lang.startsWith('zh'))
        if (zhVoice) { utterance.voice = zhVoice; utterance.lang = zhVoice.lang }
        else utterance.lang = 'zh-CN'
        utterance.rate = 1.0; utterance.pitch = 1.0; utterance.volume = 1.0

        utterance.onstart = () => console.log('[TTS] Speaking:', ttsText.substring(0, 40))
        utterance.onend = () => console.log('[TTS] Done')
        utterance.onerror = (ev: SpeechSynthesisErrorEvent) => {
          console.warn('[TTS] Error:', ev.error)
          if (ev.error !== 'canceled' && ev.error !== 'interrupted') {
            chat.addSystemMessage('🔇 语音失败: ' + ev.error)
          }
        }
        synth.speak(utterance)
        chat.addSystemMessage('🔊 语音合成')
      } catch (e: any) {
        console.error('[TTS] Exception:', e)
        chat.addSystemMessage('🔇 语音异常')
      }
    }
    // Chrome 需要在微任务中延迟一下，确保在用户手势上下文中
    setTimeout(speakFallback, 50)
  }
}

const playAnimations = (commands: any[]) => {
  const live2d = avatarRef.value
  if (!live2d) { console.warn('[Anim] avatarRef is null, skipping animations'); return }
  console.log('[Anim] Playing', commands.length, 'commands:', commands.map((c: any) => c.type || c.params?.type || '?'))
  for (const cmd of commands) {
    const params = cmd.params || cmd
    if (params.expression_name) {
      console.log('[Anim] setExpression:', params.expression_name)
      live2d.setExpression(params.expression_name, params.duration_ms || 1000)
    } else if (params.group) {
      console.log('[Anim] playMotion:', params.group)
      live2d.playMotion(params.group, params.priority || 0)
    }
  }
}

// 已发送消息的哈希集合 (用于去重)
const sentMessageHashes = new Set<string>()

const handleUserBroadcast = (broadcast: any) => {
  // 去重: 基于发送者+内容的简单哈希
  const senderName = broadcast.sender_name || '匿名用户'
  const hash = `${senderName}|${broadcast.content}`
  if (sentMessageHashes.has(hash)) return
  sentMessageHashes.add(hash)
  // 限制哈希集合大小，防止内存泄漏
  if (sentMessageHashes.size > 500) {
    const iter = sentMessageHashes.values()
    for (let i = 0; i < 200; i++) sentMessageHashes.delete(iter.next().value)
  }

  // 额外检查: 跳过最后20条中的重复消息
  const recent = chat.messages.value.slice(-20)
  const isDup = recent.some(m =>
    m.role === 'user' &&
    m.text === broadcast.content &&
    m.senderName === (broadcast.sender_name || '匿名用户')
  )
  if (isDup) return

  chat.addUserBroadcast(broadcast)
  chat.scrollToBottom(chatRef.value?.historyRef || null)
}

const handleConnectionChange = (connected: boolean) => {
  isConnected.value = connected
  connectionError.value = null
  chat.addSystemMessage(connected ? '✅ 已连接到服务器' : '❌ 连接已断开')
}

const handleError = (error: string) => {
  // 清除超时定时器
  if ((window as any).__avatarTimeout) {
    clearTimeout((window as any).__avatarTimeout)
    ;(window as any).__avatarTimeout = null
  }
  isProcessing.value = false
  if (error.toLowerCase().includes('connection') || error.toLowerCase().includes('websocket')) {
    connectionError.value = error
  }
  chat.addSystemMessage(`⚠️ ${error}`)
}


const onAnimationComplete = () => console.log('动画播放完成')

onMounted(async () => {
  ws.onAvatar(handleAvatarResponse)
  ws.onErr(handleError)
  ws.onConnectionStatusChange(handleConnectionChange)
  ws.onBroadcast(handleUserBroadcast)

  try {
    await ws.connect(currentUser.value.id)

    // 连接成功后加载历史消息（仅已登录用户）
    if (authStore.isLoggedIn && authStore.accessToken) {
      try {
        const resp = await api.get('/messages/context', {
          params: { depth: 30 }
        })
        const respData = resp.data
        if (respData.code === 200 && respData.data?.context) {
          // 清空并重建：防止 WebSocket 广播和历史加载产生重复
          chat.messages.value = []
          const serverMsgs = respData.data.context
          const currentUsername = authStore.user?.username || '我'
          // 后端返回 DESC (最新在前)，反转为 ASC (旧→新)
          const orderedMsgs = [...serverMsgs].reverse()
          for (const msg of orderedMsgs) {
            // 用户消息
            chat.messages.value.push({
              id: `hist_${msg.message_id}`,
              role: 'user',
              text: msg.message,
              senderName: currentUsername,
              timestamp: msg.created_at  // 直接传给 formatTime 自动检测格式
            })
            // Avatar 响应
            const ar = msg.avatar_response
            if (ar) {
              const arText = (typeof ar === 'object' && ar.data)
                ? (ar.data.original_text || ar.data.text || '')
                : (typeof ar === 'string' ? ar : '')
              if (arText) {
                chat.messages.value.push({
                  id: `hist_ar_${msg.message_id}`,
                  role: 'avatar',
                  text: arText,
                  timestamp: Number(msg.created_at) + 1
                })
              }
            }
          }
          chat.scrollToBottom(chatRef.value?.historyRef || null)
        }
      } catch (e: any) {
        if (e?.response?.status === 401) {
          chat.addSystemMessage('🔐 登录态已过期，请重新登录')
        } else {
          chat.addSystemMessage('📜 历史消息加载失败')
        }
      }
    } else {
      chat.addSystemMessage('💡 登录后刷新页面可以恢复历史消息（当前未登录）')
    }
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
  width: 100%;
  height: 100%;
  background:
    radial-gradient(ellipse 80% 50% at 50% -20%, rgba(108,92,231,0.15), transparent),
    radial-gradient(ellipse 50% 80% at 20% 80%, rgba(225,112,85,0.08), transparent),
    var(--c-bg);
  overflow: hidden;
}

.avatar-section {
  position: relative;
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
}

/* 状态指示器 */
.status-indicator {
  position: absolute;
  top: 12px;
  right: 420px;
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 12px;
  font-weight: 500;
  background: rgba(0,0,0,0.5);
  padding: 6px 14px;
  border-radius: 20px;
  backdrop-filter: blur(10px);
  border: 1px solid var(--c-border);
  z-index: 10;
}
.status-dot { width: 8px; height: 8px; border-radius: 50%; }
.status-dot.connected { background: var(--c-success); box-shadow: 0 0 8px rgba(0,206,201,0.5); }
.status-dot.disconnected { background: var(--c-error); }
.status-text { color: var(--c-text); opacity: 0.85; }

/* 字幕 */
.subtitle-overlay {
  position: absolute;
  bottom: 24px;
  left: 8%;
  right: 420px;
  background: linear-gradient(90deg, rgba(0,0,0,0) 0%, rgba(0,0,0,0.75) 20%, rgba(0,0,0,0.75) 80%, rgba(0,0,0,0) 100%);
  border-radius: var(--radius-sm);
  padding: 12px 24px;
  text-align: center;
  backdrop-filter: blur(8px);
  z-index: 10;
}
.subtitle-text { color: #fff; font-size: 17px; font-weight: 500; text-shadow: 0 1px 4px rgba(0,0,0,0.5); }

/* 对话框覆盖层 */
.avatar-section :deep(.chat-section) {
  position: absolute;
  right: 12px;
  top: 12px;
  bottom: 12px;
  width: 380px;
  max-height: none;
  height: auto;
  background: rgba(15,15,30,0.78);
  backdrop-filter: blur(16px) saturate(140%);
  -webkit-backdrop-filter: blur(16px) saturate(140%);
  border: 1px solid rgba(255,255,255,0.08);
  border-radius: var(--radius-lg);
  z-index: 20;
}

@media (max-width: 900px) {
  .avatar-section :deep(.chat-section) {
    right: 6px; top: 6px; bottom: 6px;
    width: 280px;
  }
  .status-indicator { right: 300px; }
  .subtitle-overlay { right: 300px; }
}
</style>
