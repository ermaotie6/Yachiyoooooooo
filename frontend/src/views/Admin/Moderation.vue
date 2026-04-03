<template>
  <div class="moderation-container">
    <div class="page-header">
      <h1>内容审核</h1>
      <el-breadcrumb :separator-icon="ArrowRight">
        <el-breadcrumb-item to="/">首页</el-breadcrumb-item>
        <el-breadcrumb-item to="/admin">管理后台</el-breadcrumb-item>
        <el-breadcrumb-item>内容审核</el-breadcrumb-item>
      </el-breadcrumb>
    </div>

    <el-card>
      <template #header>
        <div class="card-header">
          <span>待审核队列</span>
          <el-button type="primary" size="small" @click="refreshList">刷新</el-button>
        </div>
      </template>

      <el-table :data="moderationItems" stripe v-loading="loading">
        <el-table-column prop="id" label="ID" width="80" />
        <el-table-column prop="content" label="内容" min-width="300">
          <template #default="{ row }">
            <div class="content-preview">{{ row.content?.substring(0, 100) }}...</div>
          </template>
        </el-table-column>
        <el-table-column prop="violationType" label="违规类型" width="120">
          <template #default="{ row }">
            <el-tag :type="getViolationTagType(row.violationType)" size="small">
              {{ row.violationType || '待检测' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="severityScore" label="严重程度" width="120">
          <template #default="{ row }">
            <el-progress
              :percentage="(row.severityScore || 0) * 100"
              :color="getSeverityColor(row.severityScore)"
              :stroke-width="8"
            />
          </template>
        </el-table-column>
        <el-table-column prop="createdAt" label="时间" width="180">
          <template #default="{ row }">
            {{ formatDate(row.createdAt) }}
          </template>
        </el-table-column>
        <el-table-column label="操作" width="200" fixed="right">
          <template #default="{ row }">
            <el-button type="success" size="small" @click="handleApprove(row.id)">通过</el-button>
            <el-button type="warning" size="small" @click="handleWarn(row.id)">警告</el-button>
            <el-button type="danger" size="small" @click="handleBlock(row.id)">屏蔽</el-button>
          </template>
        </el-table-column>
      </el-table>

      <el-pagination
        v-model:current-page="currentPage"
        :page-size="pageSize"
        :total="total"
        layout="total, prev, pager, next"
        @current-change="handlePageChange"
        style="margin-top: 20px; justify-content: flex-end;"
      />
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { ArrowRight } from '@element-plus/icons-vue'
import { api } from '@/api/client'

const loading = ref(false)
const moderationItems = ref<any[]>([])
const currentPage = ref(1)
const pageSize = ref(20)
const total = ref(0)

const formatDate = (date: string) => {
  return date ? new Date(date).toLocaleString('zh-CN') : '-'
}

const getViolationTagType = (type: string) => {
  const map: Record<string, string> = {
    violence: 'danger', adult: 'danger', harassment: 'warning',
    spam: 'info', hate: 'danger', profanity: 'warning'
  }
  return map[type] || 'info'
}

const getSeverityColor = (score: number) => {
  if (score >= 0.8) return '#F56C6C'
  if (score >= 0.5) return '#E6A23C'
  return '#67C23A'
}

const refreshList = async () => {
  loading.value = true
  try {
    const res = await api.get('/admin/moderation/queue', {
      params: { page: currentPage.value, size: pageSize.value }
    })
    if (res.data?.data) {
      moderationItems.value = res.data.data.items || []
      total.value = res.data.data.total || 0
    }
  } catch (error) {
    console.warn('获取审核队列失败')
  } finally {
    loading.value = false
  }
}

const handleApprove = async (id: number) => {
  try {
    await api.post(`/admin/moderation/${id}/action`, { action: 'approve' })
    ElMessage.success('已通过')
    await refreshList()
  } catch { ElMessage.error('操作失败') }
}

const handleWarn = async (id: number) => {
  try {
    await api.post(`/admin/moderation/${id}/action`, { action: 'warning' })
    ElMessage.success('已发送警告')
    await refreshList()
  } catch { ElMessage.error('操作失败') }
}

const handleBlock = async (id: number) => {
  try {
    await api.post(`/admin/moderation/${id}/action`, { action: 'block' })
    ElMessage.success('已屏蔽')
    await refreshList()
  } catch { ElMessage.error('操作失败') }
}

const handlePageChange = () => {
  refreshList()
}

onMounted(() => {
  refreshList()
})
</script>

<style scoped>
.moderation-container {
  padding: 20px;
}

.page-header {
  background: white;
  padding: 20px;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  margin-bottom: 20px;
}

.page-header h1 {
  margin: 0 0 15px 0;
  color: #333;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.content-preview {
  font-size: 13px;
  color: #666;
  line-height: 1.5;
}
</style>
