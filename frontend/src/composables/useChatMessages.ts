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
      id: `sys_${Date.now()}`,
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
    emotions?: string[]
    actions?: string[]
    audio_url?: string
    audio_duration_ms?: number
  }) => {
    const msg: Message = {
      id: `msg_${Date.now()}_${Math.random().toString(36).slice(2, 8)}`,
      role: 'avatar',
      text: response.text,
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

  return {
    messages,
    addMessage,
    addSystemMessage,
    addUserBroadcast,
    addAvatarResponse,
    clearMessages,
    latestAvatarMessage,
    scrollToBottom
  }
}
