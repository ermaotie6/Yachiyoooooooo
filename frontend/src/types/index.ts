export interface User {
  userId: number
  username: string
  email: string
  displayName: string
  avatar?: string
  role: 'user' | 'admin'
  createdAt: string
}

export interface Post {
  postId: number
  userId: number
  title: string
  content: string
  contentType: string
  createdAt: string
  updatedAt?: string
  likeCount: number
  commentCount: number
  favoriteCount: number
  reviewStatus: 'pending' | 'approved' | 'rejected'
  reviewLevel: number
  tags?: string[]
}

export interface ChatMessage {
  id: number
  sessionId: string
  userMessage: string
  aiResponse: string
  timestamp: string
  processingTime: number
  // 前端扩展字段（用于渲染）
  type?: 'user' | 'ai'
  content?: string
}

export interface ChatSession {
  sessionId: string
  userId: number
  createdAt: string
  updatedAt: string
  messageCount: number
}

export interface ApiResponse<T> {
  code: number
  msg: string
  data: T
  /** @deprecated 部分旧接口可能返回 success 布尔值 */
  success?: boolean
  /** @deprecated 部分旧接口可能返回 message 字符串 */
  message?: string
  timestamp?: string
}
