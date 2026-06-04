<template>
  <Teleport to="body">
    <div v-if="localVisible" class="auth-overlay" @click.self="$emit('update:visible', false)">
      <div class="auth-dialog">
        <button class="auth-close" @click="$emit('update:visible', false)">✕</button>
        <h2>用户认证</h2>

        <!-- 标签切换 -->
        <div class="auth-tabs">
          <button :class="['tab-btn', { active: activeTab === 'login' }]" @click="activeTab = 'login'">登录</button>
          <button :class="['tab-btn', { active: activeTab === 'register' }]" @click="activeTab = 'register'">注册</button>
        </div>

        <!-- 登录表单 -->
        <form v-if="activeTab === 'login'" @submit.prevent="handleLogin" class="auth-form">
          <label>用户名</label>
          <input v-model="loginForm.username" placeholder="请输入用户名" required minlength="3" maxlength="20" />
          <label>密码</label>
          <input v-model="loginForm.password" type="password" placeholder="请输入密码" required />
          <p v-if="loginError" class="auth-error">{{ loginError }}</p>
          <button type="submit" class="btn-primary">登录</button>
        </form>

        <!-- 注册表单 -->
        <form v-if="activeTab === 'register'" @submit.prevent="handleRegister" class="auth-form">
          <label>用户名</label>
          <input v-model="registerForm.username" placeholder="3-20个字符" required minlength="3" maxlength="20" />
          <label>邮箱</label>
          <input v-model="registerForm.email" type="email" placeholder="请输入邮箱" required />
          <label>密码</label>
          <input v-model="registerForm.password" type="password" placeholder="请输入密码" required minlength="6" />
          <label>确认密码</label>
          <input v-model="registerForm.confirmPassword" type="password" placeholder="请再次输入密码" required minlength="6" />
          <p v-if="registerError" class="auth-error">{{ registerError }}</p>
          <button type="submit" class="btn-primary">注册</button>
        </form>
      </div>
    </div>
  </Teleport>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useAuthStore } from '@/stores/auth'
import { useRouter } from 'vue-router'
import { toast } from '@/utils/toast'

const props = defineProps({ visible: { type: Boolean, default: false } })
const emit = defineEmits(['close', 'update:visible'])
const localVisible = computed({ get: () => props.visible, set: (v) => emit('update:visible', v) })

const router = useRouter()
const authStore = useAuthStore()
const activeTab = ref('login')
const loginError = ref('')
const registerError = ref('')

const loginForm = ref({ username: '', password: '' })
const registerForm = ref({ username: '', email: '', password: '', confirmPassword: '' })

const handleLogin = async () => {
  loginError.value = ''
  const { username, password } = loginForm.value
  if (username.length < 3) { loginError.value = '用户名至少3个字符'; return }
  if (!password) { loginError.value = '请输入密码'; return }
  try {
    await authStore.login(username, password)
    toast.success('登录成功')
    emit('update:visible', false)
    loginForm.value = { username: '', password: '' }
    const redirect = router.currentRoute.value.query.redirect as string
    router.push(redirect || '/')
  } catch (e: any) {
    loginError.value = e.response?.data?.msg || e.response?.data?.message || '登录失败'
  }
}

const handleRegister = async () => {
  registerError.value = ''
  const { username, email, password, confirmPassword } = registerForm.value
  if (username.length < 3) { registerError.value = '用户名至少3个字符'; return }
  if (!email.includes('@')) { registerError.value = '请输入有效的邮箱'; return }
  if (password.length < 6) { registerError.value = '密码至少6个字符'; return }
  if (password !== confirmPassword) { registerError.value = '两次密码不一致'; return }
  try {
    await authStore.register(username, email, password)
    toast.success('注册成功，请登录')
    activeTab.value = 'login'
    registerForm.value = { username: '', email: '', password: '', confirmPassword: '' }
  } catch (e: any) {
    const msg = e.response?.data?.msg || e.response?.data?.message || ''
    if (msg.includes('用户名已被注册')) registerError.value = '该用户名已被注册'
    else if (msg.includes('邮箱已被注册')) registerError.value = '该邮箱已被使用'
    else registerError.value = msg || '注册失败'
  }
}
</script>

<style scoped>
.auth-overlay {
  position: fixed; inset: 0;
  background: rgba(0,0,0,0.6);
  backdrop-filter: blur(8px);
  display: flex; align-items: center; justify-content: center;
  z-index: 9999;
  animation: fadeIn .2s ease-out;
}
@keyframes fadeIn { from { opacity: 0 } to { opacity: 1 } }
.auth-dialog {
  background: var(--c-surface);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-lg);
  padding: 24px;
  width: 380px;
  max-width: 90vw;
  box-shadow: var(--shadow-lg);
  position: relative;
  animation: fadeInScale .25s var(--ease-out);
}
@keyframes fadeInScale { from { opacity:0; transform:scale(.95) } to { opacity:1; transform:scale(1) } }
.auth-close {
  position: absolute; top: 12px; right: 12px;
  background: none; border: none; color: var(--c-text-secondary);
  font-size: 20px; cursor: pointer; padding: 4px 8px; border-radius: 4px;
}
.auth-close:hover { color: var(--c-text); background: rgba(255,255,255,.06); }
h2 { color: var(--c-text); font-size: 18px; margin-bottom: 16px; }
.auth-tabs { display: flex; gap: 0; margin-bottom: 20px; border-bottom: 1px solid var(--c-border); }
.tab-btn {
  flex: 1; padding: 8px; background: none; border: none;
  color: var(--c-text-secondary); font-size: 14px; cursor: pointer;
  border-bottom: 2px solid transparent; transition: all var(--dur-fast);
}
.tab-btn.active { color: var(--c-primary-light); border-color: var(--c-primary); }
.auth-form { display: flex; flex-direction: column; gap: 8px; }
.auth-form label { font-size: 13px; color: var(--c-text-secondary); }
.auth-form input {
  padding: 10px 12px;
  background: var(--c-bg-input); border: 1px solid var(--c-border);
  border-radius: var(--radius-sm); color: var(--c-text);
  font-size: 14px; outline: none; transition: border-color var(--dur-fast);
}
.auth-form input:focus { border-color: var(--c-primary); }
.auth-error { color: var(--c-error); font-size: 13px; }
.btn-primary {
  margin-top: 4px; padding: 10px;
  background: linear-gradient(135deg, var(--c-primary), var(--c-primary-dark));
  color: #fff; border: none; border-radius: var(--radius-sm);
  font-size: 14px; font-weight: 600; cursor: pointer;
  transition: all var(--dur-fast) var(--ease-out);
}
.btn-primary:hover { transform: translateY(-1px); box-shadow: 0 4px 20px rgba(108,92,231,.4); }
</style>
