import { ref } from 'vue'

interface Message {
  type: string
  data?: Record<string, any>
}

interface AvatarResponse {
  request_id: string
  text: string
  audio_url: string
  audio_duration_ms: number
  emotions?: string[]
  actions?: string[]
  animation_commands?: Record<string, any>[]
  timestamp: number
}

interface StatusUpdate {
  status: string
  progress: number
  message: string
  details?: Record<string, any>
  timestamp: number
}

export function useWebSocket() {
  // 连接状态
  const isConnected = ref(false)
  const isConnecting = ref(false)
  const connectionError = ref<string | null>(null)

  // WebSocket 实例
  let ws: WebSocket | null = null

  // 消息队列 (离线缓冲)
  const messageQueue = ref<Message[]>([])

  // 重连配置
  const reconnectAttempts = ref(0)
  const maxReconnectAttempts = 5
  const reconnectDelay = ref(3000)

  // 用户广播消息类型
  type UserBroadcast = {
    sender_id: string
    sender_name: string
    content: string
    timestamp: number
  }

  // 事件回调
  let onAvatarResponse: ((response: AvatarResponse) => void) | null = null
  let onStatusUpdate: ((status: StatusUpdate) => void) | null = null
  let onError: ((error: string) => void) | null = null
  let onConnectionChanged: ((connected: boolean) => void) | null = null
  let onUserBroadcast: ((broadcast: UserBroadcast) => void) | null = null

  /**
   * 连接到 WebSocket 服务器
   */
  const connect = (userId: string): Promise<void> => {
    return new Promise((resolve, reject) => {
      if (isConnected.value) {
        resolve()
        return
      }

      if (isConnecting.value) {
        reject(new Error('Already connecting'))
        return
      }

      isConnecting.value = true
      connectionError.value = null
      intentionalDisconnect = false

      // 获取 WebSocket URL
      // 开发环境: Vite 代理 /ws -> ws://localhost:9001
      // 生产环境: Nginx 代理 /ws -> websocket_server:9001
      // 也支持直接通过 VITE_WS_BASE_URL 指定完整 URL
      const envWsUrl = import.meta.env.VITE_WS_BASE_URL || ''
      let wsUrl: string
      if (envWsUrl && (envWsUrl.startsWith('ws://') || envWsUrl.startsWith('wss://'))) {
        // 显式配置了完整 WebSocket URL (如 ws://localhost:9001)
        wsUrl = envWsUrl
      } else {
        // 使用相对路径，由 Vite 代理 (开发) 或 Nginx (生产) 处理
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
        const host = window.location.host
        wsUrl = `${protocol}//${host}/ws`
      }
      const url = `${wsUrl}?user_id=${userId}`

      console.log('[WebSocket] Connecting to', url)

      let settled = false

      try {
        ws = new WebSocket(url)

        ws.onopen = () => {
          console.log('[WebSocket] Connected')
          isConnected.value = true
          isConnecting.value = false
          connectionError.value = null
          reconnectAttempts.value = 0
          reconnectDelay.value = 3000

          // 发送身份识别消息，让后端关联 user_id
          sendMessage({
            type: 'identify',
            data: {
              user_id: userId,
              device_type: 'web'
            }
          })

          if (onConnectionChanged) {
            onConnectionChanged(true)
          }

          // 刷新离线消息队列
          flushMessageQueue()

          if (!settled) { settled = true; resolve() }
        }

        ws.onmessage = (event: MessageEvent) => {
          handleMessage(event.data)
        }

        ws.onerror = (event: Event) => {
          console.error('[WebSocket] Error:', event)
          connectionError.value = 'WebSocket error'
          if (onError) {
            onError('WebSocket connection error')
          }
        }

        ws.onclose = () => {
          console.log('[WebSocket] Disconnected')
          isConnected.value = false
          isConnecting.value = false

          if (onConnectionChanged) {
            onConnectionChanged(false)
          }

          // 只有在非主动断开时才尝试重连
          if (!intentionalDisconnect) {
            attemptReconnect(userId)
          }

          if (!settled) { settled = true; reject(new Error('WebSocket closed')) }
        }
      } catch (error) {
        isConnecting.value = false
        connectionError.value = String(error)
        if (onError) {
          onError(String(error))
        }
        if (!settled) { settled = true; reject(error) }
      }
    })
  }

  // 是否主动断开（避免触发重连）
  let intentionalDisconnect = false

  /**
   * 断开连接
   */
  const disconnect = (): Promise<void> => {
    return new Promise((resolve) => {
      intentionalDisconnect = true
      if (ws) {
        ws.close()
        ws = null
      }
      isConnected.value = false
      resolve()
    })
  }

  /**
   * 处理接收到的消息
   */
  const handleMessage = (data: string) => {
    try {
      const message = JSON.parse(data)
      console.log('[WebSocket] Received:', message.type, message)

      switch (message.type) {
        case 'welcome':
          // 服务器欢迎消息，包含分配的 client_id
          console.log('[WebSocket] Welcome, client_id:', message.data?.client_id)
          break

        case 'ping':
          // 响应心跳
          sendMessage({
            type: 'pong',
            data: {
              timestamp: Date.now()
            }
          })
          break

        case 'avatar_response':
          if (onAvatarResponse && message.data) {
            const response: AvatarResponse = {
              request_id: message.data.request_id || '',
              text: message.data.text || '',
              audio_url: message.data.audio_url || '',
              audio_duration_ms: message.data.audio_duration_ms || 0,
              emotions: message.data.emotions || [],
              actions: message.data.actions || [],
              animation_commands: message.data.animation_commands || [],
              timestamp: message.data.timestamp || Date.now()
            }
            onAvatarResponse(response)
          }
          break

        case 'status':
          if (onStatusUpdate && message.data) {
            const status: StatusUpdate = {
              status: message.data.status || '',
              progress: message.data.progress || 0,
              message: message.data.message || '',
              details: message.data.details,
              timestamp: message.data.timestamp || Date.now()
            }
            onStatusUpdate(status)
          }
          break

        case 'notification':
          if (onError && message.data) {
            const notification = message.data
            onError(`[${notification.level}] ${notification.title}: ${notification.message}`)
          }
          break

        case 'user_broadcast':
          // 其他用户发送的消息广播
          if (onUserBroadcast && message.data) {
            const broadcast: UserBroadcast = {
              sender_id: message.data.sender_id || '',
              sender_name: message.data.sender_name || '匿名用户',
              content: message.data.content || '',
              timestamp: message.data.timestamp || Date.now()
            }
            onUserBroadcast(broadcast)
          }
          break

        case 'error':
          if (onError && message.data) {
            onError(message.data.message || 'Unknown error')
          }
          break

        default:
          console.warn('[WebSocket] Unknown message type:', message.type)
      }
    } catch (error) {
      console.error('[WebSocket] Error parsing message:', error)
    }
  }

  /**
   * 发送消息 (内部方法)
   */
  const sendMessage = (message: Message): boolean => {
    if (!ws || ws.readyState !== WebSocket.OPEN) {
      // 连接未就绪，加入队列
      messageQueue.value.push(message)
      console.log('[WebSocket] Message queued (not connected)')
      return false
    }

    try {
      ws.send(JSON.stringify(message))
      console.log('[WebSocket] Sent:', message.type)
      return true
    } catch (error) {
      console.error('[WebSocket] Error sending message:', error)
      messageQueue.value.push(message)
      return false
    }
  }

  /**
   * 发送用户消息
   */
  const sendUserMessage = (content: string, userId?: string, username?: string): boolean => {
    // 获取 access_token 用于 WebSocket 消息认证
    let accessToken = ''
    try {
      accessToken = localStorage.getItem('accessToken') || ''
    } catch { /* 静默忽略 */ }

    return sendMessage({
      type: 'user_message',
      data: {
        content,
        user_id: userId || '',
        username: username || '',
        access_token: accessToken,  // 携带JWT令牌供后端验证身份
        timestamp: Date.now(),
        language: 'ja'  // 目标语言：日语（百度翻译 API 翻译为日语）
      }
    })
  }

  /**
   * 刷新消息队列
   */
  const flushMessageQueue = () => {
    const pending = [...messageQueue.value]
    messageQueue.value = []

    for (const message of pending) {
      if (!sendMessage(message)) {
        // 发送失败，将剩余消息放回队列
        messageQueue.value.push(message)
        break
      }
    }
  }

  /**
   * 尝试重连
   */
  const attemptReconnect = (userId: string) => {
    if (reconnectAttempts.value >= maxReconnectAttempts) {
      console.error(
        '[WebSocket] Max reconnect attempts reached',
        maxReconnectAttempts
      )
      connectionError.value = 'Connection failed after multiple attempts'
      return
    }

    reconnectAttempts.value++
    const delay = reconnectDelay.value * Math.pow(1.5, reconnectAttempts.value - 1)

    console.log(
      `[WebSocket] Reconnecting in ${delay}ms (attempt ${reconnectAttempts.value}/${maxReconnectAttempts})`
    )

    setTimeout(() => {
      connect(userId).catch((error) => {
        console.error('[WebSocket] Reconnection failed:', error)
      })
    }, delay)
  }

  /**
   * 设置事件监听器
   */
  const onAvatar = (callback: (response: AvatarResponse) => void) => {
    onAvatarResponse = callback
  }

  const onStatus = (callback: (status: StatusUpdate) => void) => {
    onStatusUpdate = callback
  }

  const onErr = (callback: (error: string) => void) => {
    onError = callback
  }

  const onConnectionStatusChange = (callback: (connected: boolean) => void) => {
    onConnectionChanged = callback
  }

  const onBroadcast = (callback: (broadcast: { sender_id: string; sender_name: string; content: string; timestamp: number }) => void) => {
    onUserBroadcast = callback
  }

  return {
    // 状态
    isConnected,
    isConnecting,
    connectionError,

    // 方法
    connect,
    disconnect,
    sendUserMessage,
    sendMessage,

    // 事件监听
    onAvatar,
    onStatus,
    onErr,
    onConnectionStatusChange,
    onBroadcast
  }
}
