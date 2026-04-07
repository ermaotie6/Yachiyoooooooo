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
}

export interface ChatSession {
  sessionId: string
  userId: number
  createdAt: string
  updatedAt: string
  messageCount: number
}

export interface ApiResponse<T> {
  success: boolean
  code: string
  message: string
  data: T
  timestamp: string
}
