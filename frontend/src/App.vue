<template>
  <div class="app-container">
    <div class="app-header">
      <div class="logo">
        <span class="logo-text">Yachiyoの小屋</span>
      </div>
      <div class="nav-right">
        <button v-if="!authStore.isLoggedIn" class="login-btn" @click="authDialogVisible = true">
          登录 / 注册
        </button>
        <div v-else class="user-menu">
          <span class="user-name">{{ authStore.user?.username }}</span>
          <button class="logout-btn" @click="handleLogout">退出</button>
        </div>
      </div>
    </div>

    <div class="app-main">
      <router-view />
    </div>

    <AuthDialog v-model:visible="authDialogVisible" />
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useAuthStore } from '@/stores/auth'
import { toast } from '@/utils/toast'
import AuthDialog from '@/components/AuthDialog.vue'

const authStore = useAuthStore()
const authDialogVisible = ref(false)

const handleLogout = async () => {
  try {
    if (authStore.refreshToken) {
      const { api } = await import('@/api/client')
      await api.post('/auth/logout', { refresh_token: authStore.refreshToken })
    }
  } catch { /* 即使后端失败也清理本地状态 */ }
  authStore.logout()
  toast.success('已退出登录')
}
</script>

<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
html, body, #app { height: 100%; width: 100%; overflow: hidden; }
</style>

<style scoped>
.app-container {
  display: flex; flex-direction: column;
  height: 100vh; background: var(--c-bg);
}
.app-header {
  display: flex; align-items: center; justify-content: space-between;
  height: 46px; padding: 0 16px; flex-shrink: 0; z-index: 50;
  background: var(--c-surface); border-bottom: 1px solid var(--c-border);
}
.logo-text { font-size: 17px; font-weight: 700; color: var(--c-primary-light); letter-spacing: 0.5px; }
.nav-right { display: flex; align-items: center; gap: 10px; }
.login-btn {
  padding: 6px 14px; background: linear-gradient(135deg, var(--c-primary), var(--c-primary-dark));
  color: #fff; border: none; border-radius: var(--radius-sm); font-size: 13px; font-weight: 600;
  cursor: pointer; transition: all var(--dur-fast);
}
.login-btn:hover { transform: translateY(-1px); box-shadow: 0 2px 12px rgba(108,92,231,.3); }
.user-menu { display: flex; align-items: center; gap: 8px; }
.user-name { font-size: 13px; color: var(--c-text); }
.logout-btn {
  padding: 4px 10px; background: rgba(255,118,117,.15); color: var(--c-error);
  border: 1px solid rgba(255,118,117,.25); border-radius: var(--radius-sm);
  font-size: 12px; cursor: pointer; transition: all var(--dur-fast);
}
.logout-btn:hover { background: rgba(255,118,117,.25); }
.app-main { flex: 1; overflow: hidden; }
</style>
