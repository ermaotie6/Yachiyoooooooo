<template>
  <div class="profile-container">
    <div class="profile-card">
      <div class="profile-header">
        <div class="avatar">
          <el-avatar :size="100" :src="user.avatar" />
        </div>
        <div class="user-info">
          <h1>{{ user.username }}</h1>
          <p>{{ user.displayName }}</p>
          <el-tag>{{ getRoleText(user.role) }}</el-tag>
        </div>
        <el-button type="primary" @click="editMode = true">编辑资料</el-button>
      </div>

      <el-divider />

      <div class="profile-details">
        <el-row :gutter="20">
          <el-col :xs="24" :sm="12" :md="6">
            <div class="stat-item">
              <div class="stat-label">邮箱</div>
              <div class="stat-value">{{ user.email }}</div>
            </div>
          </el-col>
          <el-col :xs="24" :sm="12" :md="6">
            <div class="stat-item">
              <div class="stat-label">加入时间</div>
              <div class="stat-value">{{ formatDate(user.createdAt) }}</div>
            </div>
          </el-col>
          <el-col :xs="24" :sm="12" :md="6">
            <div class="stat-item">
              <div class="stat-label">发布内容</div>
              <div class="stat-value">{{ stats.postCount }}</div>
            </div>
          </el-col>
          <el-col :xs="24" :sm="12" :md="6">
            <div class="stat-item">
              <div class="stat-label">获赞总数</div>
              <div class="stat-value">{{ stats.totalLikes }}</div>
            </div>
          </el-col>
        </el-row>
      </div>

      <el-divider />

      <div class="user-posts">
        <h2>我的内容</h2>
        <el-empty v-if="userPosts.length === 0" description="暂无内容" />
        <div v-else class="posts-grid">
          <div v-for="post in userPosts" :key="post.postId" class="post-item">
            <h3>{{ post.title }}</h3>
            <p>{{ post.content.substring(0, 80) }}...</p>
            <div class="post-stats">
              <span>👍 {{ post.likeCount }}</span>
              <span>💬 {{ post.commentCount }}</span>
            </div>
            <router-link :to="`/posts/${post.postId}`">
              <el-button type="primary" text>查看</el-button>
            </router-link>
          </div>
        </div>
      </div>
    </div>

    <!-- 编辑对话框 -->
    <el-dialog
      v-model="editMode"
      title="编辑个人资料"
      width="500px"
    >
      <el-form :model="editForm">
        <el-form-item label="用户名">
          <el-input v-model="editForm.username" disabled />
        </el-form-item>
        <el-form-item label="显示名称">
          <el-input v-model="editForm.displayName" placeholder="输入显示名称" />
        </el-form-item>
        <el-form-item label="邮箱">
          <el-input v-model="editForm.email" type="email" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="editMode = false">取消</el-button>
        <el-button type="primary" @click="saveProfile">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { useAuthStore } from '@/stores/auth'
import { api } from '@/api/client'
import type { User, Post } from '@/types'

const authStore = useAuthStore()
const editMode = ref(false)

const user = ref<User>({
  userId: 0,
  username: '',
  email: '',
  displayName: '',
  role: 'user',
  createdAt: ''
})

const editForm = ref({
  username: '',
  displayName: '',
  email: ''
})

const stats = ref({
  postCount: 0,
  totalLikes: 0
})

const userPosts = ref<Post[]>([])

const getRoleText = (role: string) => {
  const roleMap: Record<string, string> = {
    user: '普通用户',
    broadcaster: '主播',
    moderator: '版主',
    admin: '管理员'
  }
  return roleMap[role] || '用户'
}

const formatDate = (date: string) => {
  return new Date(date).toLocaleDateString('zh-CN')
}

const saveProfile = async () => {
  try {
    await api.put('/auth/profile', {
      displayName: editForm.value.displayName,
      email: editForm.value.email
    })
    ElMessage.success('资料已保存')
    editMode.value = false
    user.value.displayName = editForm.value.displayName
    user.value.email = editForm.value.email
    // 同步更新 auth store 中的用户信息
    authStore.setUser({ ...user.value })
  } catch (error) {
    ElMessage.error('保存失败')
  }
}

onMounted(async () => {
  if (authStore.user) {
    user.value = { ...authStore.user }
    editForm.value = {
      username: user.value.username,
      displayName: user.value.displayName,
      email: user.value.email
    }
  }

  // 加载用户统计数据
  try {
    const statsRes = await api.get(`/users/${user.value.userId}/stats`)
    if (statsRes.data?.data) {
      stats.value = statsRes.data.data
    }
  } catch (error) {
    console.warn('获取用户统计失败，使用默认值')
  }

  // 加载用户发布的内容
  try {
    const postsRes = await api.get(`/users/${user.value.userId}/posts`)
    if (postsRes.data?.data) {
      userPosts.value = postsRes.data.data
    }
  } catch (error) {
    console.warn('获取用户内容失败')
  }
})
</script>

<style scoped>
.profile-container {
  max-width: 1000px;
  margin: 0 auto;
  padding: 20px;
}

.profile-card {
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  padding: 40px;
}

.profile-header {
  display: flex;
  align-items: center;
  gap: 30px;
  margin-bottom: 20px;
}

.avatar {
  flex-shrink: 0;
}

.user-info {
  flex: 1;
}

.user-info h1 {
  margin: 0 0 10px 0;
  font-size: 28px;
  color: #333;
}

.user-info p {
  margin: 0 0 10px 0;
  color: #666;
  font-size: 14px;
}

.profile-details {
  margin: 30px 0;
}

.stat-item {
  text-align: center;
  padding: 20px;
  background: #f9f9f9;
  border-radius: 8px;
}

.stat-label {
  font-size: 12px;
  color: #999;
  margin-bottom: 8px;
}

.stat-value {
  font-size: 24px;
  font-weight: bold;
  color: #667eea;
}

.user-posts {
  margin-top: 30px;
}

.user-posts h2 {
  margin-bottom: 20px;
  font-size: 20px;
  color: #333;
}

.posts-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
  gap: 15px;
}

.post-item {
  padding: 15px;
  border: 1px solid #eee;
  border-radius: 6px;
  transition: box-shadow 0.3s;
}

.post-item:hover {
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.post-item h3 {
  margin: 0 0 10px 0;
  font-size: 14px;
  color: #333;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.post-item p {
  margin: 0 0 10px 0;
  font-size: 12px;
  color: #666;
  overflow: hidden;
  text-overflow: ellipsis;
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
}

.post-stats {
  display: flex;
  gap: 15px;
  font-size: 12px;
  color: #999;
  margin-bottom: 10px;
}

@media (max-width: 768px) {
  .profile-header {
    flex-direction: column;
    text-align: center;
  }

  .profile-card {
    padding: 20px;
  }
}
</style>
