// Yachiyo 精简类型定义

export interface User {
  userId: number
  username: string
  email: string
  displayName: string
  avatar?: string
  role: 'user' | 'admin'
  createdAt: string
}

export interface ChatMessage {
  id: number
  sessionId: string
  userMessage: string
  aiResponse: string
  timestamp: string
  processingTime: number
  type?: 'user' | 'ai'
  content?: string
}

export interface ApiResponse<T> {
  code: number
  msg: string
  data: T
  success?: boolean
  message?: string
  timestamp?: string
}
