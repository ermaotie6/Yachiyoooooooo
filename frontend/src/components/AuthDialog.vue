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
        <el-form ref="loginFormRef" :model="loginForm" :rules="loginRules" @submit.prevent="handleLogin">
          <el-form-item label="用户名" prop="username">
            <el-input v-model="loginForm.username" placeholder="请输入用户名" />
          </el-form-item>
          <el-form-item label="密码" prop="password">
            <el-input
              v-model="loginForm.password"
              type="password"
              placeholder="请输入密码"
              show-password
            />
          </el-form-item>
          <el-button type="primary" class="full-width" @click="handleLogin">
            登录
          </el-button>
        </el-form>
      </el-tab-pane>

      <!-- 注册选项卡 -->
      <el-tab-pane label="注册" name="register">
        <el-form ref="registerFormRef" :model="registerForm" :rules="registerRules" @submit.prevent="handleRegister">
          <el-form-item label="用户名" prop="username">
            <el-input v-model="registerForm.username" placeholder="请输入用户名" />
          </el-form-item>
          <el-form-item label="邮箱" prop="email">
            <el-input v-model="registerForm.email" type="email" placeholder="请输入邮箱" />
          </el-form-item>
          <el-form-item label="密码" prop="password">
            <el-input
              v-model="registerForm.password"
              type="password"
              placeholder="请输入密码"
              show-password
            />
          </el-form-item>
          <el-form-item label="确认密码" prop="confirmPassword">
            <el-input
              v-model="registerForm.confirmPassword"
              type="password"
              placeholder="请再次输入密码"
              show-password
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
import { ref, reactive } from 'vue'
import { ElMessage, type FormInstance, type FormRules } from 'element-plus'
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
const loginFormRef = ref<FormInstance>()
const registerFormRef = ref<FormInstance>()

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

const loginRules = reactive<FormRules>({
  username: [
    { required: true, message: '请输入用户名', trigger: 'blur' },
    { min: 3, max: 20, message: '用户名长度在 3 到 20 个字符', trigger: 'blur' }
  ],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 8, message: '密码至少 8 个字符', trigger: 'blur' }
  ]
})

const validateConfirmPassword = (_rule: any, value: string, callback: Function) => {
  if (value !== registerForm.value.password) {
    callback(new Error('两次密码输入不一致'))
  } else {
    callback()
  }
}

const registerRules = reactive<FormRules>({
  username: [
    { required: true, message: '请输入用户名', trigger: 'blur' },
    { min: 3, max: 20, message: '用户名长度在 3 到 20 个字符', trigger: 'blur' }
  ],
  email: [
    { required: true, message: '请输入邮箱地址', trigger: 'blur' },
    { type: 'email', message: '请输入正确的邮箱地址', trigger: 'blur' }
  ],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 8, message: '密码至少 8 个字符', trigger: 'blur' }
  ],
  confirmPassword: [
    { required: true, message: '请再次输入密码', trigger: 'blur' },
    { validator: validateConfirmPassword, trigger: 'blur' }
  ]
})

const handleLogin = async () => {
  if (!loginFormRef.value) return
  await loginFormRef.value.validate(async (valid) => {
    if (!valid) return
    try {
      await authStore.login(loginForm.value.username, loginForm.value.password)
      ElMessage.success('登录成功')
      emit('close')
      // 如果有 redirect 参数，跳转到目标页面；否则默认跳转到直播页面
      const redirect = router.currentRoute.value.query.redirect as string
      router.push(redirect || '/livestream')
    } catch (error: any) {
      ElMessage.error(error.response?.data?.msg || error.response?.data?.message || '登录失败')
    }
  })
}

const handleRegister = async () => {
  if (!registerFormRef.value) return
  await registerFormRef.value.validate(async (valid) => {
    if (!valid) return
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
      const msg = error.response?.data?.msg || error.response?.data?.message || ''
      if (msg.includes('用户名已被注册') || msg.includes('username') && msg.includes('exist')) {
        ElMessage.error('该用户名已被注册，请换一个用户名')
      } else if (msg.includes('邮箱已被注册') || msg.includes('email') && msg.includes('exist')) {
        ElMessage.error('该邮箱已被注册，请使用其他邮箱')
      } else {
        ElMessage.error(msg || '注册失败')
      }
    }
  })
}
</script>

<style scoped>
.full-width {
  width: 100%;
}
</style>
