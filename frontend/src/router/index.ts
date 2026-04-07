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
      path: '/livestream',
      component: () => import('@/views/LiveStream.vue'),
      meta: { title: '直播间', requiresAuth: true }
    },
    {
      path: '/livestream/:id',
      component: () => import('@/views/LiveStream.vue'),
      meta: { title: '直播间', requiresAuth: true }
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
    },
    {
      path: '/admin/moderation',
      component: () => import('@/views/Admin/Moderation.vue'),
      meta: { title: '内容审核', requiresAuth: true, requiresAdmin: true }
    },
    {
      path: '/admin/messages',
      component: () => import('@/views/Admin/MessageManagement.vue'),
      meta: { title: '消息管理', requiresAuth: true, requiresAdmin: true }
    },
    {
      path: '/admin/users',
      component: () => import('@/views/Admin/UserManagement.vue'),
      meta: { title: '用户管理', requiresAuth: true, requiresAdmin: true }
    }
  ]
})

// 路由守卫
router.beforeEach((to, from, next) => {
  const authStore = useAuthStore()
  
  if (to.meta.requiresAuth && !authStore.isLoggedIn) {
    // 重定向到登录页面，记住要返回的页面
    next({
      path: '/',
      query: { redirect: to.fullPath }
    })
    return
  }
  
  if (to.meta.requiresAdmin && authStore.user?.role !== 'admin') {
    next('/')
    return
  }
  
  document.title = `${to.meta.title || '页面'} - Yachiyo`
  next()
})

// 路由后置守卫 - 用于处理页面加载完成后的逻辑
router.afterEach((to) => {
  // 如果进入直播间，初始化 WebSocket 连接
  if (to.path.startsWith('/livestream')) {
    // WebSocket 连接将在 LiveStream 组件中初始化
    console.log('进入直播间页面，将建立 WebSocket 连接')
  }
})

export default router
