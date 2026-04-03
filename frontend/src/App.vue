<template>
  <div class="app-container">
    <el-container>
      <el-header class="app-header">
        <div class="header-content">
          <div class="logo">
            <span class="logo-icon">🎭</span>
            <span class="logo-text">Yachiyo</span>
          </div>
          <div class="nav-center">
            <router-link to="/" class="nav-link">首页</router-link>
            <router-link to="/livestream" class="nav-link">直播间</router-link>
            <router-link to="/posts" class="nav-link">内容</router-link>
            <router-link to="/chat" class="nav-link">AI聊天</router-link>
          </div>
          <div class="nav-right">
            <el-button v-if="!authStore.isLoggedIn" type="primary" text @click="showAuthDialog">
              登录/注册
            </el-button>
            <el-dropdown v-else>
              <span class="el-dropdown-link">
                {{ authStore.user?.username }}
                <el-icon class="el-icon--right">
                  <arrow-down />
                </el-icon>
              </span>
              <template #dropdown>
                <el-dropdown-menu>
                  <el-dropdown-item @click="goToProfile">个人资料</el-dropdown-item>
                  <el-dropdown-item @click="handleLogout">退出登录</el-dropdown-item>
                </el-dropdown-menu>
              </template>
            </el-dropdown>
          </div>
        </div>
      </el-header>

      <el-main class="app-main">
        <router-view v-slot="{ Component }">
          <component :is="Component" />
        </router-view>
      </el-main>
    </el-container>

    <!-- 认证对话框 -->
    <AuthDialog :visible="authDialogVisible" @close="authDialogVisible = false" />
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { ElMessage } from 'element-plus'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { ArrowDown } from '@element-plus/icons-vue'
import AuthDialog from '@/components/AuthDialog.vue'

const router = useRouter()
const authStore = useAuthStore()
const authDialogVisible = ref(false)

const showAuthDialog = () => {
  authDialogVisible.value = true
}

const goToProfile = () => {
  router.push('/profile')
}

const handleLogout = () => {
  authStore.logout()
  ElMessage.success('已退出登录')
  router.push('/')
}
</script>

<style scoped>
.app-container {
  min-height: 100vh;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

.app-header {
  background: rgba(255, 255, 255, 0.95) !important;
  border-bottom: 1px solid #e0e0e0;
  padding: 0 !important;
  display: flex;
  align-items: center;
}

.header-content {
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 40px;
}

.logo {
  display: flex;
  align-items: center;
  font-size: 24px;
  font-weight: bold;
  cursor: pointer;
}

.logo-icon {
  font-size: 32px;
  margin-right: 12px;
}

.logo-text {
  color: #667eea;
}

.nav-center {
  display: flex;
  gap: 30px;
  flex: 1;
  justify-content: center;
}

.nav-link {
  color: #333;
  text-decoration: none;
  font-size: 16px;
  padding: 0 10px;
  transition: color 0.3s;
}

.nav-link:hover {
  color: #667eea;
}

.nav-link.router-link-active {
  color: #667eea;
  border-bottom: 2px solid #667eea;
  padding-bottom: 2px;
}

.nav-right {
  display: flex;
  align-items: center;
  gap: 20px;
}

.el-dropdown-link {
  cursor: pointer;
  color: #333;
  display: flex;
  align-items: center;
}

.app-main {
  padding: 40px;
  max-width: 1400px;
  margin: 0 auto;
}
</style>
