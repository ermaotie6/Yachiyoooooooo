import { createApp } from 'vue'
import { createPinia } from 'pinia'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import 'element-plus/theme-chalk/dark/css-vars.css'

import App from './App.vue'
import router from './router'
import './styles/main.css'

const app = createApp(App)
const pinia = createPinia()

app.use(pinia)
app.use(router)
app.use(ElementPlus)

// 从 localStorage 恢复登录状态，等待验证完成后再挂载应用
// 避免路由守卫在 token 验证完成前误判用户未登录
import { useAuthStore } from '@/stores/auth'
const authStore = useAuthStore(pinia)
authStore.initializeAuth().finally(() => {
  app.mount('#app')
})
