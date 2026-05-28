import { createRouter, createWebHistory } from 'vue-router'
import type { Router } from 'vue-router'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/',
      component: () => import('@/views/LiveStream.vue'),
      meta: { title: 'Yachiyo 直播间' }
    }
  ]
})

// 路由守卫 — 未登录时允许访问（弹窗提示登录）
router.beforeEach((to, _from, next) => {
  document.title = `${to.meta.title || 'Yachiyo'}`
  next()
})

export default router
