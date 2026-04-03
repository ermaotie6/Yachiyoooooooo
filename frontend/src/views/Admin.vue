<template>
  <div class="admin-container">
    <div class="admin-header">
      <h1>管理后台</h1>
      <el-breadcrumb :separator-icon="ArrowRight">
        <el-breadcrumb-item to="/">首页</el-breadcrumb-item>
        <el-breadcrumb-item>管理后台</el-breadcrumb-item>
      </el-breadcrumb>
    </div>

    <el-tabs v-model="activeTab">
      <!-- 待审核内容 -->
      <el-tab-pane label="待审核内容" name="pending">
        <div class="tab-content">
          <el-table :data="pendingPosts" stripe>
            <el-table-column prop="postId" label="ID" width="80" />
            <el-table-column prop="title" label="标题" min-width="200" />
            <el-table-column prop="content" label="内容摘要" min-width="200">
              <template #default="{ row }">
                {{ row.content.substring(0, 50) }}...
              </template>
            </el-table-column>
            <el-table-column prop="createdAt" label="发布时间" width="180">
              <template #default="{ row }">
                {{ formatDate(row.createdAt) }}
              </template>
            </el-table-column>
            <el-table-column label="操作" width="150">
              <template #default="{ row }">
                <el-button type="success" size="small" @click="approvePost(row.postId)">
                  通过
                </el-button>
                <el-button type="danger" size="small" @click="rejectPost(row.postId)">
                  拒绝
                </el-button>
              </template>
            </el-table-column>
          </el-table>
        </div>
      </el-tab-pane>

      <!-- 统计数据 -->
      <el-tab-pane label="统计数据" name="statistics">
        <div class="tab-content">
          <el-row :gutter="20">
            <el-col :xs="24" :sm="12" :md="6">
              <div class="stat-card">
                <div class="stat-number">{{ stats.totalUsers }}</div>
                <div class="stat-label">总用户数</div>
              </div>
            </el-col>
            <el-col :xs="24" :sm="12" :md="6">
              <div class="stat-card">
                <div class="stat-number">{{ stats.totalPosts }}</div>
                <div class="stat-label">总内容数</div>
              </div>
            </el-col>
            <el-col :xs="24" :sm="12" :md="6">
              <div class="stat-card">
                <div class="stat-number">{{ stats.pendingReview }}</div>
                <div class="stat-label">待审核</div>
              </div>
            </el-col>
            <el-col :xs="24" :sm="12" :md="6">
              <div class="stat-card">
                <div class="stat-number">{{ stats.reviewRate }}%</div>
                <div class="stat-label">审核通过率</div>
              </div>
            </el-col>
          </el-row>
        </div>
      </el-tab-pane>

      <!-- 系统设置 -->
      <el-tab-pane label="系统设置" name="settings">
        <div class="tab-content">
          <el-form :model="settings" label-width="150px">
            <el-form-item label="平台名称">
              <el-input v-model="settings.platformName" />
            </el-form-item>
            <el-form-item label="最大上传大小(MB)">
              <el-input-number v-model="settings.maxUploadSize" :min="1" :max="1000" />
            </el-form-item>
            <el-form-item label="内容审核级别">
              <el-select v-model="settings.reviewLevel">
                <el-option label="严格" value="strict" />
                <el-option label="中等" value="medium" />
                <el-option label="宽松" value="loose" />
              </el-select>
            </el-form-item>
            <el-form-item>
              <el-button type="primary" @click="saveSettings">保存设置</el-button>
            </el-form-item>
          </el-form>
        </div>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { ArrowRight } from '@element-plus/icons-vue'
import { api } from '@/api/client'
import type { Post } from '@/types'

const activeTab = ref('pending')

const pendingPosts = ref<Post[]>([])
const stats = ref({
  totalUsers: 0,
  totalPosts: 0,
  pendingReview: 0,
  reviewRate: 0
})

const settings = ref({
  platformName: 'Yachiyo',
  maxUploadSize: 100,
  reviewLevel: 'medium'
})

const formatDate = (date: string) => {
  return new Date(date).toLocaleDateString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit'
  })
}

const approvePost = async (postId: number) => {
  try {
    await api.post(`/admin/posts/${postId}/review`, {
      status: 'approved'
    })
    ElMessage.success('已通过审核')
    pendingPosts.value = pendingPosts.value.filter(p => p.postId !== postId)
  } catch (error) {
    ElMessage.error('操作失败')
  }
}

const rejectPost = async (postId: number) => {
  try {
    await api.post(`/admin/posts/${postId}/review`, {
      status: 'rejected'
    })
    ElMessage.success('已拒绝')
    pendingPosts.value = pendingPosts.value.filter(p => p.postId !== postId)
  } catch (error) {
    ElMessage.error('操作失败')
  }
}

const saveSettings = async () => {
  try {
    await api.put('/admin/settings', settings.value)
    ElMessage.success('设置已保存')
  } catch (error) {
    ElMessage.error('保存设置失败')
  }
}

onMounted(async () => {
  // 并行加载待审核内容和统计数据
  try {
    const [pendingRes, statsRes] = await Promise.allSettled([
      api.get('/admin/posts/pending'),
      api.get('/admin/stats')
    ])
    if (pendingRes.status === 'fulfilled') {
      pendingPosts.value = pendingRes.value.data?.data?.posts || []
    }
    if (statsRes.status === 'fulfilled' && statsRes.value.data?.data) {
      stats.value = statsRes.value.data.data
    }
  } catch (error) {
    console.error('加载管理后台数据失败')
  }
})
</script>

<style scoped>
.admin-container {
  padding: 20px;
}

.admin-header {
  background: white;
  padding: 20px;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  margin-bottom: 20px;
}

.admin-header h1 {
  margin: 0 0 15px 0;
  color: #333;
}

.tab-content {
  padding: 20px;
  background: white;
  border-radius: 8px;
  margin-top: 10px;
}

.stat-card {
  background: white;
  padding: 25px;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  text-align: center;
}

.stat-number {
  font-size: 32px;
  font-weight: bold;
  color: #667eea;
  margin-bottom: 10px;
}

.stat-label {
  font-size: 14px;
  color: #666;
}

:deep(.el-form-item) {
  max-width: 400px;
}
</style>
