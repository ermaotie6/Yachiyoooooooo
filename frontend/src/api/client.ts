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

// 响应拦截器
api.interceptors.response.use(
  response => response,
  async error => {
    const authStore = useAuthStore()
    
    if (error.response?.status === 401) {
      // Token 过期，尝试刷新
      if (authStore.refreshToken) {
        try {
          await authStore.refreshAccessToken()
          // 重试原请求
          return api.request(error.config)
        } catch (refreshError) {
          authStore.logout()
          ElMessage.error('登录已过期，请重新登录')
          window.location.href = '/'
        }
      }
    }
    
    ElMessage.error(error.response?.data?.message || '请求失败')
    return Promise.reject(error)
  }
)

export { api }
