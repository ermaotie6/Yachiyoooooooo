import { createRouter, createWebHistory } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import type { Router } from 'vue-router'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/',
      component: () => import('@/views/Home.vue'),
      meta: { title: '首页' }
    },
    {
      path: '/posts',
      component: () => import('@/views/Posts.vue'),
      meta: { title: '内容' }
    },
    {
      path: '/posts/:id',
      component: () => import('@/views/PostDetail.vue'),
      meta: { title: '内容详情' }
    },
    {
      path: '/chat',
      component: () => import('@/views/Chat.vue'),
      meta: { title: 'AI聊天', requiresAuth: true }
    },
    {
      path: '/profile',
      component: () => import('@/views/Profile.vue'),
      meta: { title: '个人资料', requiresAuth: true }
    },
    {
      path: '/admin',
      component: () => import('@/views/Admin.vue'),
      meta: { title: '管理后台', requiresAuth: true, requiresAdmin: true }
    }
  ]
})

// 路由守卫
router.beforeEach((to, from, next) => {
  const authStore = useAuthStore()
  
  if (to.meta.requiresAuth && !authStore.isLoggedIn) {
    next('/')
    return
  }
  
  if (to.meta.requiresAdmin && authStore.user?.role !== 'admin') {
    next('/')
    return
  }
  
  document.title = `${to.meta.title || '页面'} - Yachiyo`
  next()
})

export default router
