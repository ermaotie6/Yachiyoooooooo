import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { User } from '@/types'
import { api } from '@/api/client'

export const useAuthStore = defineStore('auth', () => {
  const user = ref<User | null>(null)
  const accessToken = ref<string>(localStorage.getItem('accessToken') || '')
  const refreshToken = ref<string>(localStorage.getItem('refreshToken') || '')

  const isLoggedIn = computed(() => !!accessToken.value && !!user.value)

  const setTokens = (access: string, refresh: string) => {
    accessToken.value = access
    refreshToken.value = refresh
    localStorage.setItem('accessToken', access)
    localStorage.setItem('refreshToken', refresh)
  }

  const setUser = (userData: User) => {
    user.value = userData
    localStorage.setItem('user', JSON.stringify(userData))
  }

  const login = async (username: string, password: string) => {
    const response = await api.post('/auth/login', { username, password })
    const data = response.data.data
    // 后端返回格式: { access_token, refresh_token, user: { id, username, email, role } }
    const access = data.access_token || data.accessToken || data.token || ''
    const refresh = data.refresh_token || data.refreshToken || ''
    setTokens(access, refresh)
    
    // 构建用户对象 — 后端 user 子对象用 id (不是 userId)，role 可能是数字字符串
    const backendUser = data.user || {}
    const roleRaw = String(backendUser.role ?? data.role ?? 'user').toLowerCase()
    const normalizedRole: 'user' | 'admin' = (roleRaw === '99' || roleRaw === 'admin') ? 'admin' : 'user'
    
    const userData: User = {
      userId: backendUser.id ?? data.userId ?? data.user_id ?? 0,
      username: backendUser.username ?? data.username ?? username,
      email: backendUser.email ?? data.email ?? '',
      displayName: backendUser.nickname ?? backendUser.username ?? data.username ?? username,
      role: normalizedRole,
      createdAt: backendUser.created_at ?? data.createdAt ?? new Date().toISOString()
    }
    setUser(userData)
    return response.data
  }

  const register = async (username: string, email: string, password: string) => {
    const response = await api.post('/auth/register', { username, email, password })
    return response.data
  }

  const logout = () => {
    user.value = null
    accessToken.value = ''
    refreshToken.value = ''
    localStorage.removeItem('accessToken')
    localStorage.removeItem('refreshToken')
    localStorage.removeItem('user')
  }

  const refreshAccessToken = async () => {
    const response = await api.post('/auth/refresh', { refresh_token: refreshToken.value })
    const data = response.data.data
    const access = data.access_token || data.accessToken || data.token || ''
    const refresh = data.refresh_token || data.refreshToken || refreshToken.value
    setTokens(access, refresh)
    return access
  }

  // 初始化：从存储恢复用户
  const initializeAuth = () => {
    const savedUser = localStorage.getItem('user')
    if (savedUser) {
      user.value = JSON.parse(savedUser)
    }
  }

  return {
    user,
    accessToken,
    refreshToken,
    isLoggedIn,
    setTokens,
    setUser,
    login,
    register,
    logout,
    refreshAccessToken,
    initializeAuth
  }
})
