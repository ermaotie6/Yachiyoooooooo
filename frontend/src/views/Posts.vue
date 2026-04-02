<template>
  <div class="posts-container">
    <div class="posts-header">
      <h1>内容中心</h1>
      <el-button
        v-if="authStore.isLoggedIn"
        type="primary"
        @click="showCreateDialog = true"
      >
        发布内容
      </el-button>
    </div>

    <!-- 发布内容对话框 -->
    <el-dialog
      v-model="showCreateDialog"
      title="发布新内容"
      width="600px"
    >
      <el-form :model="newPost">
        <el-form-item label="标题">
          <el-input v-model="newPost.title" placeholder="输入内容标题" />
        </el-form-item>
        <el-form-item label="内容">
          <el-input
            v-model="newPost.content"
            type="textarea"
            :rows="6"
            placeholder="输入内容描述"
          />
        </el-form-item>
        <el-form-item label="标签">
          <el-input
            v-model="tagInput"
            placeholder="输入标签后按回车添加"
            @keyup.enter="addTag"
          />
          <div class="tag-list">
            <el-tag
              v-for="tag in newPost.tags"
              :key="tag"
              closable
              @close="removeTag(tag)"
            >
              {{ tag }}
            </el-tag>
          </div>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showCreateDialog = false">取消</el-button>
        <el-button type="primary" @click="handleCreatePost">发布</el-button>
      </template>
    </el-dialog>

    <!-- 内容列表 -->
    <el-empty v-if="postStore.posts.length === 0" description="暂无内容" />
    <div v-else class="posts-list">
      <div v-for="post in postStore.posts" :key="post.postId" class="post-card">
        <div class="post-header">
          <h2>{{ post.title }}</h2>
          <el-tag :type="getReviewStatusType(post.reviewStatus)">
            {{ getReviewStatusText(post.reviewStatus) }}
          </el-tag>
        </div>
        <p class="post-content">{{ post.content }}</p>
        <div class="post-meta">
          <span>创建于：{{ formatDate(post.createdAt) }}</span>
          <span v-if="post.tags && post.tags.length > 0">
            <el-tag v-for="tag in post.tags" :key="tag" size="small">
              {{ tag }}
            </el-tag>
          </span>
        </div>
        <div class="post-actions">
          <el-statistic title="点赞" :value="post.likeCount" />
          <el-statistic title="评论" :value="post.commentCount" />
          <el-statistic title="收藏" :value="post.favoriteCount" />
          <router-link :to="`/posts/${post.postId}`">
            <el-button type="primary" text>查看详情</el-button>
          </router-link>
        </div>
      </div>
    </div>

    <!-- 分页 -->
    <div class="pagination">
      <el-pagination
        v-model:current-page="currentPage"
        v-model:page-size="pageSize"
        :page-sizes="[10, 20, 50]"
        :total="total"
        layout="total, sizes, prev, pager, next, jumper"
        @change="fetchPosts"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { useAuthStore } from '@/stores/auth'
import { usePostStore } from '@/stores/post'

const authStore = useAuthStore()
const postStore = usePostStore()

const showCreateDialog = ref(false)
const currentPage = ref(1)
const pageSize = ref(10)
const total = ref(0)

const newPost = ref({
  title: '',
  content: '',
  tags: [] as string[]
})

const tagInput = ref('')

const addTag = () => {
  if (tagInput.value && !newPost.value.tags.includes(tagInput.value)) {
    newPost.value.tags.push(tagInput.value)
    tagInput.value = ''
  }
}

const removeTag = (tag: string) => {
  newPost.value.tags = newPost.value.tags.filter(t => t !== tag)
}

const fetchPosts = async () => {
  try {
    const data = await postStore.fetchPosts(currentPage.value, pageSize.value)
    total.value = data.pagination.total
  } catch (error) {
    ElMessage.error('获取内容列表失败')
  }
}

const handleCreatePost = async () => {
  if (!newPost.value.title || !newPost.value.content) {
    ElMessage.warning('请输入标题和内容')
    return
  }

  try {
    await postStore.createPost(
      newPost.value.title,
      newPost.value.content,
      newPost.value.tags
    )
    ElMessage.success('内容发布成功')
    showCreateDialog.value = false
    newPost.value = { title: '', content: '', tags: [] }
    fetchPosts()
  } catch (error: any) {
    ElMessage.error(error.response?.data?.message || '发布失败')
  }
}

const getReviewStatusType = (status: string) => {
  const typeMap: Record<string, string> = {
    pending: 'warning',
    approved: 'success',
    rejected: 'danger'
  }
  return typeMap[status] || 'info'
}

const getReviewStatusText = (status: string) => {
  const textMap: Record<string, string> = {
    pending: '待审核',
    approved: '已通过',
    rejected: '已拒绝'
  }
  return textMap[status] || '未知'
}

const formatDate = (date: string) => {
  return new Date(date).toLocaleDateString('zh-CN')
}

onMounted(() => {
  fetchPosts()
})
</script>

<style scoped>
.posts-container {
  padding: 20px;
}

.posts-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 30px;
  padding: 20px;
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.posts-header h1 {
  margin: 0;
  color: #333;
}

.posts-list {
  display: grid;
  gap: 20px;
}

.post-card {
  background: white;
  padding: 20px;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  transition: box-shadow 0.3s;
}

.post-card:hover {
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.15);
}

.post-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 15px;
  padding-bottom: 15px;
  border-bottom: 1px solid #eee;
}

.post-header h2 {
  margin: 0;
  font-size: 20px;
  color: #333;
}

.post-content {
  color: #666;
  line-height: 1.6;
  margin-bottom: 15px;
  max-height: 100px;
  overflow: hidden;
  text-overflow: ellipsis;
}

.post-meta {
  display: flex;
  gap: 20px;
  font-size: 12px;
  color: #999;
  margin-bottom: 15px;
}

.tag-list {
  margin-top: 10px;
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.post-actions {
  display: flex;
  gap: 30px;
  align-items: center;
  padding-top: 15px;
  border-top: 1px solid #eee;
}

.pagination {
  margin-top: 30px;
  text-align: center;
}
</style>
