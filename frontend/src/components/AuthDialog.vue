<template>
  <el-dialog
    v-model="visible"
    title="用户认证"
    width="400px"
    @close="$emit('close')"
  >
    <el-tabs v-model="activeTab">
      <!-- 登录选项卡 -->
      <el-tab-pane label="登录" name="login">
        <el-form ref="loginFormRef" :model="loginForm" @submit.prevent="handleLogin">
          <el-form-item label="用户名">
            <el-input v-model="loginForm.username" placeholder="请输入用户名" />
          </el-form-item>
          <el-form-item label="密码">
            <el-input
              v-model="loginForm.password"
              type="password"
              placeholder="请输入密码"
            />
          </el-form-item>
          <el-button type="primary" class="full-width" @click="handleLogin">
            登录
          </el-button>
        </el-form>
      </el-tab-pane>

      <!-- 注册选项卡 -->
      <el-tab-pane label="注册" name="register">
        <el-form ref="registerFormRef" :model="registerForm" @submit.prevent="handleRegister">
          <el-form-item label="用户名">
            <el-input v-model="registerForm.username" placeholder="请输入用户名" />
          </el-form-item>
          <el-form-item label="邮箱">
            <el-input v-model="registerForm.email" type="email" placeholder="请输入邮箱" />
          </el-form-item>
          <el-form-item label="密码">
            <el-input
              v-model="registerForm.password"
              type="password"
              placeholder="请输入密码"
            />
          </el-form-item>
          <el-form-item label="确认密码">
            <el-input
              v-model="registerForm.confirmPassword"
              type="password"
              placeholder="请再次输入密码"
            />
          </el-form-item>
          <el-button type="primary" class="full-width" @click="handleRegister">
            注册
          </el-button>
        </el-form>
      </el-tab-pane>
    </el-tabs>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { ElMessage } from 'element-plus'
import { useAuthStore } from '@/stores/auth'
import { useRouter } from 'vue-router'

const props = defineProps({
  visible: {
    type: Boolean,
    default: false
  }
})

const emit = defineEmits(['close'])

const router = useRouter()
const authStore = useAuthStore()

const activeTab = ref('login')

const loginForm = ref({
  username: '',
  password: ''
})

const registerForm = ref({
  username: '',
  email: '',
  password: '',
  confirmPassword: ''
})

const handleLogin = async () => {
  if (!loginForm.value.username || !loginForm.value.password) {
    ElMessage.warning('请输入用户名和密码')
    return
  }

  try {
    await authStore.login(loginForm.value.username, loginForm.value.password)
    ElMessage.success('登录成功')
    emit('close')
    router.push('/')
  } catch (error: any) {
    ElMessage.error(error.response?.data?.message || '登录失败')
  }
}

const handleRegister = async () => {
  if (!registerForm.value.username || !registerForm.value.email || !registerForm.value.password) {
    ElMessage.warning('请填写所有必填项')
    return
  }

  if (registerForm.value.password !== registerForm.value.confirmPassword) {
    ElMessage.warning('两次密码输入不一致')
    return
  }

  try {
    await authStore.register(
      registerForm.value.username,
      registerForm.value.email,
      registerForm.value.password
    )
    ElMessage.success('注册成功，请登录')
    activeTab.value = 'login'
    registerForm.value = {
      username: '',
      email: '',
      password: '',
      confirmPassword: ''
    }
  } catch (error: any) {
    ElMessage.error(error.response?.data?.message || '注册失败')
  }
}
</script>

<style scoped>
.full-width {
  width: 100%;
}
</style>
