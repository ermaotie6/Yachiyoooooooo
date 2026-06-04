import axios from 'axios'
import { useAuthStore } from '@/stores/auth'
import { toast } from '@/utils/toast'

const api = axios.create({
  baseURL: '/api/v1',
  timeout: 10000,
  headers: { 'Content-Type': 'application/json' }
})

api.interceptors.request.use(config => {
  const token = useAuthStore().accessToken
  if (token) config.headers.Authorization = `Bearer ${token}`
  return config
}, error => Promise.reject(error))

let isRefreshing = false
let pendingRequests: Array<(token: string) => void> = []

const processQueue = (token: string) => {
  pendingRequests.forEach(cb => cb(token))
  pendingRequests = []
}

api.interceptors.response.use(
  response => response,
  async error => {
    const authStore = useAuthStore()
    const originalRequest = error.config
    if (error.response?.status === 401 && !originalRequest._retry) {
      const url = originalRequest.url || ''
      if (url.includes('/auth/refresh') || url.includes('/auth/logout')) return Promise.reject(error)
      if (isRefreshing) {
        return new Promise(resolve => {
          pendingRequests.push((token: string) => {
            originalRequest.headers.Authorization = `Bearer ${token}`
            resolve(api.request(originalRequest))
          })
        })
      }
      originalRequest._retry = true
      if (authStore.refreshToken) {
        isRefreshing = true
        try {
          const newToken = await authStore.refreshAccessToken()
          processQueue(newToken)
          originalRequest.headers.Authorization = `Bearer ${newToken}`
          return api.request(originalRequest)
        } catch {
          authStore.logout()
          pendingRequests = []
        } finally { isRefreshing = false }
      }
    }
    if (error.response?.status !== 401) {
      toast.error(error.response?.data?.message || '请求失败')
    }
    return Promise.reject(error)
  }
)

export { api }

const apiV2 = axios.create({
  baseURL: '/api/v2',
  timeout: 30000,
  headers: { 'Content-Type': 'application/json' }
})

apiV2.interceptors.request.use(config => {
  const token = useAuthStore().accessToken
  if (token) config.headers.Authorization = `Bearer ${token}`
  return config
}, error => Promise.reject(error))

apiV2.interceptors.response.use(
  response => response,
  async error => {
    const authStore = useAuthStore()
    const originalRequest = error.config
    if (error.response?.status === 401 && !originalRequest._retry) {
      if (isRefreshing) {
        return new Promise(resolve => {
          pendingRequests.push((token: string) => {
            originalRequest.headers.Authorization = `Bearer ${token}`
            const instance = originalRequest.baseURL?.includes('/v2') ? apiV2 : api
            resolve(instance.request(originalRequest))
          })
        })
      }
      originalRequest._retry = true
      if (authStore.refreshToken) {
        isRefreshing = true
        try {
          const newToken = await authStore.refreshAccessToken()
          processQueue(newToken)
          originalRequest.headers.Authorization = `Bearer ${newToken}`
          return apiV2.request(originalRequest)
        } catch {
          authStore.logout()
          pendingRequests = []
        } finally { isRefreshing = false }
      }
    }
    if (error.response?.status !== 401) {
      toast.error(error.response?.data?.message || '请求失败')
    }
    return Promise.reject(error)
  }
)

export { apiV2 }
