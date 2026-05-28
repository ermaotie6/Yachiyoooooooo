<template>
  <div class="app-container">
    <div class="app-header">
      <div class="logo">
        <img src="/images/logo.svg" alt="Yachiyo" class="logo-icon" />
        <span class="logo-text">Yachiyo</span>
      </div>
      <div class="nav-right">
        <el-button v-if="!authStore.isLoggedIn" type="primary" size="small" @click="authDialogVisible = true">
          登录 / 注册
        </el-button>
        <el-dropdown v-else>
          <span class="user-name">
            {{ authStore.user?.username }}
            <el-icon><arrow-down /></el-icon>
          </span>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item @click="handleLogout">退出登录</el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
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
import { ElMessage } from 'element-plus'
import { useAuthStore } from '@/stores/auth'
import { ArrowDown } from '@element-plus/icons-vue'
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
  ElMessage.success('已退出登录')
}
</script>

<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
html, body, #app { height: 100%; width: 100%; overflow: hidden; }
</style>

<style scoped>
.app-container {
  display: flex;
  flex-direction: column;
  height: 100vh;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

.app-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 50px;
  padding: 0 20px;
  background: rgba(255, 255, 255, 0.95);
  border-bottom: 1px solid #e0e0e0;
  flex-shrink: 0;
  z-index: 10;
}

.logo {
  display: flex;
  align-items: center;
  gap: 8px;
}

.logo-icon {
  width: 28px;
  height: 28px;
  object-fit: contain;
}

.logo-text {
  font-size: 18px;
  font-weight: bold;
  color: #667eea;
}

.nav-right {
  display: flex;
  align-items: center;
}

.user-name {
  cursor: pointer;
  color: #333;
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 14px;
}

.app-main {
  flex: 1;
  overflow: hidden;
}
</style>
