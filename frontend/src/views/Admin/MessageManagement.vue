<template>
  <div class="message-management-container">
    <div class="page-header">
      <h1>消息管理</h1>
      <el-breadcrumb :separator-icon="ArrowRight">
        <el-breadcrumb-item to="/">首页</el-breadcrumb-item>
        <el-breadcrumb-item to="/admin">管理后台</el-breadcrumb-item>
        <el-breadcrumb-item>消息管理</el-breadcrumb-item>
      </el-breadcrumb>
    </div>

    <el-card>
      <template #header>
        <div class="card-header">
          <span>消息列表</span>
          <div class="header-actions">
            <el-select v-model="statusFilter" placeholder="筛选状态" clearable size="small" @change="fetchMessages">
              <el-option label="全部" value="" />
              <el-option label="已通过" value="approved" />
              <el-option label="待审核" value="pending" />
              <el-option label="已拒绝" value="rejected" />
            </el-select>
            <el-input v-model="searchKeyword" placeholder="搜索消息..." clearable size="small" style="width: 200px; margin-left: 10px;" @keyup.enter="fetchMessages" />
          </div>
        </div>
      </template>

      <el-table :data="messages" stripe v-loading="loading">
        <el-table-column prop="id" label="ID" width="80" />
        <el-table-column prop="user_id" label="用户ID" width="120" />
        <el-table-column prop="message" label="内容" min-width="300">
          <template #default="{ row }">
            {{ row.message?.substring(0, 80) }}{{ row.message?.length > 80 ? '...' : '' }}
          </template>
        </el-table-column>
        <el-table-column prop="review_status" label="状态" width="100">
          <template #default="{ row }">
            <el-tag :type="getStatusType(row.review_status)" size="small">
              {{ getStatusText(row.review_status) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="created_at" label="时间" width="180">
          <template #default="{ row }">
            {{ formatDate(row.created_at) }}
          </template>
        </el-table-column>
        <el-table-column label="操作" width="150" fixed="right">
          <template #default="{ row }">
            <el-button type="danger" size="small" @click="deleteMessage(row.message_id)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>

      <el-pagination
        v-model:current-page="currentPage"
        :page-size="pageSize"
        :total="total"
        layout="total, prev, pager, next"
        @current-change="fetchMessages"
        style="margin-top: 20px; justify-content: flex-end;"
      />
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { ArrowRight } from '@element-plus/icons-vue'
import { api } from '@/api/client'

const loading = ref(false)
const messages = ref<any[]>([])
const currentPage = ref(1)
const pageSize = ref(20)
const total = ref(0)
const statusFilter = ref('')
const searchKeyword = ref('')

const formatDate = (date: string) => date ? new Date(date).toLocaleString('zh-CN') : '-'

const getStatusType = (status: number | string) => {
  // 后端 review_status: 0=PENDING, 1=APPROVED, 2=REJECTED, 3=MANUAL_REVIEW
  const map: Record<string, string> = { '1': 'success', '0': 'warning', '2': 'danger', '3': 'warning', approved: 'success', pending: 'warning', rejected: 'danger' }
  return map[String(status)] || 'info'
}

const getStatusText = (status: number | string) => {
  const map: Record<string, string> = { '1': '已通过', '0': '待审核', '2': '已拒绝', '3': '人工审核', approved: '已通过', pending: '待审核', rejected: '已拒绝' }
  return map[String(status)] || '未知'
}

const fetchMessages = async () => {
  loading.value = true
  try {
    const res = await api.get('/messages/list', {
      params: { page: currentPage.value, size: pageSize.value, status: statusFilter.value, keyword: searchKeyword.value }
    })
    if (res.data?.data) {
      // 后端返回 { data: { messages: [...], total: N } }
      messages.value = res.data.data.messages || res.data.data.items || []
      total.value = res.data.data.total || 0
    }
  } catch (error) {
    console.warn('获取消息列表失败')
  } finally {
    loading.value = false
  }
}

const deleteMessage = async (id: number) => {
  try {
    await ElMessageBox.confirm('确定要删除这条消息吗？', '确认删除', { type: 'warning' })
    await api.post('/messages/delete', { message_id: id })
    ElMessage.success('已删除')
    await fetchMessages()
  } catch (error) {
    if (error !== 'cancel') ElMessage.error('删除失败')
  }
}

onMounted(() => {
  fetchMessages()
})
</script>

<style scoped>
.message-management-container {
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

.header-actions {
  display: flex;
  align-items: center;
}
</style>
