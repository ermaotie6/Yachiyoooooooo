import axios from 'axios'
import { useAuthStore } from '@/stores/auth'
import { ElMessage } from 'element-plus'

const api = axios.create({
  baseURL: '/api/v1',
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json'
  }
})

// 请求拦截器
api.interceptors.request.use(
  config => {
    const authStore = useAuthStore()
    if (authStore.accessToken) {
      config.headers.Authorization = `Bearer ${authStore.accessToken}`
    }
    return config
  },
  error => Promise.reject(error)
)

// Token 刷新并发锁
let isRefreshing = false
let pendingRequests: Array<(token: string) => void> = []

const processQueue = (token: string) => {
  pendingRequests.forEach(cb => cb(token))
  pendingRequests = []
}

// 响应拦截器
api.interceptors.response.use(
  response => response,
  async error => {
    const authStore = useAuthStore()
    const originalRequest = error.config
    
    if (error.response?.status === 401 && !originalRequest._retry) {
      if (isRefreshing) {
        // 已在刷新中，排队等待
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
        } catch (refreshError) {
          authStore.logout()
          pendingRequests = []
          ElMessage.error('登录已过期，请重新登录')
          window.location.href = '/'
        } finally {
          isRefreshing = false
        }
      }
    }
    
    if (error.response?.status !== 401) {
      ElMessage.error(error.response?.data?.message || '请求失败')
    }
    return Promise.reject(error)
  }
)

export { api }

// API v2 instance (for AI endpoints)
const apiV2 = axios.create({
  baseURL: '/api/v2',
  timeout: 30000, // AI requests may take longer
  headers: {
    'Content-Type': 'application/json'
  }
})

// v2 也需要请求拦截器
apiV2.interceptors.request.use(
  config => {
    const authStore = useAuthStore()
    if (authStore.accessToken) {
      config.headers.Authorization = `Bearer ${authStore.accessToken}`
    }
    return config
  },
  error => Promise.reject(error)
)

// v2 使用独立的响应拦截器（确保重试请求使用 apiV2 实例而非 api）
apiV2.interceptors.response.use(
  response => response,
  async error => {
    const authStore = useAuthStore()
    const originalRequest = error.config
    
    if (error.response?.status === 401 && !originalRequest._retry) {
      if (isRefreshing) {
        // 已在刷新中，排队等待（注意：使用 apiV2 实例重试）
        return new Promise(resolve => {
          pendingRequests.push((token: string) => {
            originalRequest.headers.Authorization = `Bearer ${token}`
            // 根据原始请求的 baseURL 判断应该用哪个实例重试
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
        } catch (refreshError) {
          authStore.logout()
          pendingRequests = []
          ElMessage.error('登录已过期，请重新登录')
          window.location.href = '/'
        } finally {
          isRefreshing = false
        }
      }
    }
    
    if (error.response?.status !== 401) {
      ElMessage.error(error.response?.data?.message || '请求失败')
    }
    return Promise.reject(error)
  }
)

export { apiV2 }
