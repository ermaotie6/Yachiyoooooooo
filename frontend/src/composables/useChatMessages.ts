import { ref, computed, nextTick } from 'vue'

export interface Message {
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

export function useChatMessages() {
  const messages = ref<Message[]>([])

  const addMessage = (msg: Message) => {
    messages.value.push(msg)
  }

  const addSystemMessage = (text: string) => {
    messages.value.push({
      id: `sys_${Date.now()}_${Math.random().toString(36).slice(2, 6)}`,
      role: 'system',
      text,
      timestamp: Date.now()
    })
  }

  const addUserBroadcast = (broadcast: {
    sender_id: string
    sender_name: string
    content: string
    timestamp: number
  }) => {
    messages.value.push({
      id: `msg_${broadcast.timestamp}_${Math.random().toString(36).slice(2, 8)}`,
      role: 'user',
      text: broadcast.content,
      timestamp: broadcast.timestamp,
      senderName: broadcast.sender_name
    })
  }

  const addAvatarResponse = (response: {
    text: string
    original_text?: string
    emotions?: string[]
    actions?: string[]
    audio_url?: string
    audio_duration_ms?: number
  }) => {
    const msg: Message = {
      id: `msg_${Date.now()}_${Math.random().toString(36).slice(2, 8)}`,
      role: 'avatar',
      text: response.original_text || response.text,
      emotions: response.emotions || [],
      actions: response.actions || [],
      audioUrl: response.audio_url,
      audioDuration: response.audio_duration_ms,
      timestamp: Date.now()
    }
    messages.value.push(msg)
    return msg
  }

  const clearMessages = () => {
    messages.value = []
  }

  const latestAvatarMessage = computed(() => {
    const reversed = [...messages.value].reverse()
    return reversed.find(m => m.role === 'avatar') || null
  })

  const scrollToBottom = (el: HTMLElement | null) => {
    nextTick(() => {
      if (el) el.scrollTop = el.scrollHeight
    })
  }

  // 从后端历史记录加载消息（刷新页面后恢复，仅登录用户有效）
  const loadFromHistory = (serverMessages: Array<{
    message_id: number
    user_id: number
    message: string
    avatar_response?: string
    created_at: number
  }>) => {
    if (!serverMessages || serverMessages.length === 0) return

    // 先清空，防止重复加载
    messages.value = []

    // 按时间排序（旧消息在前）
    const sorted = [...serverMessages].sort((a, b) => a.created_at - b.created_at)

    for (const msg of sorted) {
      // 用户消息
      messages.value.push({
        id: `hist_${msg.message_id}`,
        role: 'user',
        text: msg.message,
        senderName: `用户${msg.user_id}`,
        timestamp: msg.created_at
      })

      // 如果有 avatar 回复
      if (msg.avatar_response) {
        try {
          const resp = typeof msg.avatar_response === 'string'
            ? JSON.parse(msg.avatar_response)
            : msg.avatar_response
          const arData = resp?.data || resp
          messages.value.push({
            id: `hist_resp_${msg.message_id}`,
            role: 'avatar',
            text: arData.original_text || arData.text || '',
            emotions: arData.emotions || [],
            timestamp: Number(msg.created_at) + 1  // 数字类型，formatTime 自动检测
          })
        } catch {
          // avatar_response 解析失败，静默忽略
        }
      }
    }
  }

  return {
    messages,
    addMessage,
    addSystemMessage,
    addUserBroadcast,
    addAvatarResponse,
    clearMessages,
    latestAvatarMessage,
    scrollToBottom,
    loadFromHistory
  }
}
