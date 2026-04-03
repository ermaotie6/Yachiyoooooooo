<template>
  <div class="user-management-container">
    <div class="page-header">
      <h1>用户管理</h1>
      <el-breadcrumb :separator-icon="ArrowRight">
        <el-breadcrumb-item to="/">首页</el-breadcrumb-item>
        <el-breadcrumb-item to="/admin">管理后台</el-breadcrumb-item>
        <el-breadcrumb-item>用户管理</el-breadcrumb-item>
      </el-breadcrumb>
    </div>

    <el-card>
      <template #header>
        <div class="card-header">
          <span>用户列表</span>
          <el-input v-model="searchKeyword" placeholder="搜索用户..." clearable size="small" style="width: 250px;" @keyup.enter="fetchUsers" />
        </div>
      </template>

      <el-table :data="users" stripe v-loading="loading">
        <el-table-column prop="userId" label="ID" width="80" />
        <el-table-column prop="username" label="用户名" width="150" />
        <el-table-column prop="email" label="邮箱" min-width="200" />
        <el-table-column prop="displayName" label="显示名称" width="150" />
        <el-table-column prop="role" label="角色" width="120">
          <template #default="{ row }">
            <el-tag :type="getRoleType(row.role)" size="small">{{ getRoleText(row.role) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="isActive" label="状态" width="100">
          <template #default="{ row }">
            <el-tag :type="row.isActive ? 'success' : 'danger'" size="small">
              {{ row.isActive ? '正常' : '禁用' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="createdAt" label="注册时间" width="180">
          <template #default="{ row }">
            {{ formatDate(row.createdAt) }}
          </template>
        </el-table-column>
        <el-table-column label="操作" width="200" fixed="right">
          <template #default="{ row }">
            <el-select
              :model-value="row.role"
              size="small"
              style="width: 90px; margin-right: 5px;"
              @change="(val: string) => changeRole(row.userId, val)"
            >
              <el-option label="用户" value="user" />
              <el-option label="主播" value="broadcaster" />
              <el-option label="版主" value="moderator" />
              <el-option label="管理" value="admin" />
            </el-select>
            <el-button
              :type="row.isActive ? 'danger' : 'success'"
              size="small"
              @click="toggleUserStatus(row.userId, row.isActive)"
            >
              {{ row.isActive ? '禁用' : '启用' }}
            </el-button>
          </template>
        </el-table-column>
      </el-table>

      <el-pagination
        v-model:current-page="currentPage"
        :page-size="pageSize"
        :total="total"
        layout="total, prev, pager, next"
        @current-change="fetchUsers"
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
const users = ref<any[]>([])
const currentPage = ref(1)
const pageSize = ref(20)
const total = ref(0)
const searchKeyword = ref('')

const formatDate = (date: string) => date ? new Date(date).toLocaleString('zh-CN') : '-'

const getRoleType = (role: string) => {
  const map: Record<string, string> = { admin: 'danger', moderator: 'warning', broadcaster: 'success', user: 'info' }
  return map[role] || 'info'
}

const getRoleText = (role: string) => {
  const map: Record<string, string> = { admin: '管理员', moderator: '版主', broadcaster: '主播', user: '用户' }
  return map[role] || '用户'
}

const fetchUsers = async () => {
  loading.value = true
  try {
    const res = await api.get('/admin/users', {
      params: { page: currentPage.value, size: pageSize.value, keyword: searchKeyword.value }
    })
    if (res.data?.data) {
      users.value = res.data.data.items || []
      total.value = res.data.data.total || 0
    }
  } catch (error) {
    console.warn('获取用户列表失败')
  } finally {
    loading.value = false
  }
}

const changeRole = async (userId: number, newRole: string) => {
  try {
    await ElMessageBox.confirm(`确定要将该用户角色更改为 ${getRoleText(newRole)} 吗？`, '确认更改', { type: 'warning' })
    await api.put(`/admin/users/${userId}/role`, { role: newRole })
    ElMessage.success('角色已更新')
    await fetchUsers()
  } catch (error) {
    if (error !== 'cancel') ElMessage.error('更新失败')
  }
}

const toggleUserStatus = async (userId: number, isActive: boolean) => {
  const action = isActive ? '禁用' : '启用'
  try {
    await ElMessageBox.confirm(`确定要${action}该用户吗？`, `确认${action}`, { type: 'warning' })
    await api.put(`/admin/users/${userId}/status`, { isActive: !isActive })
    ElMessage.success(`已${action}`)
    await fetchUsers()
  } catch (error) {
    if (error !== 'cancel') ElMessage.error('操作失败')
  }
}

onMounted(() => {
  fetchUsers()
})
</script>

<style scoped>
.user-management-container {
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
</style>
