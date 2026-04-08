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
    // 兼容 SMALLINT 值 "99" 和字符串 "admin"
    const normalizedRole: 'user' | 'admin' = (roleRaw === 'admin' || roleRaw === '99') ? 'admin' : 'user'
    
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

  // 初始化：从存储恢复用户，并验证 token 有效性
  const initializeAuth = async () => {
    const savedUser = localStorage.getItem('user')
    if (savedUser) {
      try {
        user.value = JSON.parse(savedUser)
      } catch {
        // 解析失败，清除脏数据
        logout()
        return
      }
    }

    // 如果有 accessToken，验证其有效性
    if (accessToken.value && user.value) {
      try {
        // 尝试使用当前 token 获取用户信息来验证有效性
        const response = await api.get('/auth/me')
        if (response.data?.data) {
          // token 有效，使用服务端返回的最新用户信息
          const serverUser = response.data.data
          const roleRaw = String(serverUser.role ?? 'user').toLowerCase()
          // 兼容 SMALLINT 值 "99" 和字符串 "admin"
          const normalizedRole: 'user' | 'admin' = (roleRaw === 'admin' || roleRaw === '99') ? 'admin' : 'user'
          const userData: User = {
            userId: serverUser.id ?? user.value.userId,
            username: serverUser.username ?? user.value.username,
            email: serverUser.email ?? user.value.email,
            displayName: serverUser.nickname ?? serverUser.username ?? user.value.displayName,
            role: normalizedRole,
            createdAt: serverUser.created_at ?? user.value.createdAt
          }
          setUser(userData)
        }
      } catch {
        // token 无效或过期，尝试刷新
        if (refreshToken.value) {
          try {
            await refreshAccessToken()
          } catch {
            // 刷新也失败，强制登出
            logout()
          }
        } else {
          logout()
        }
      }
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
