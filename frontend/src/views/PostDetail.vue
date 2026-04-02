<template>
  <div class="post-detail-container">
    <el-skeleton v-if="postStore.loading" :rows="5" animated />
    <div v-else-if="postStore.currentPost" class="post-detail">
      <div class="post-header">
        <h1>{{ postStore.currentPost.title }}</h1>
        <div class="post-meta">
          <span>发布于：{{ formatDate(postStore.currentPost.createdAt) }}</span>
          <el-tag :type="getReviewStatusType(postStore.currentPost.reviewStatus)">
            {{ getReviewStatusText(postStore.currentPost.reviewStatus) }}
          </el-tag>
        </div>
      </div>

      <div class="post-body">
        {{ postStore.currentPost.content }}
      </div>

      <div class="post-tags">
        <el-tag v-for="tag in postStore.currentPost.tags" :key="tag">
          {{ tag }}
        </el-tag>
      </div>

      <div class="post-actions">
        <el-button
          :type="isLiked ? 'primary' : 'default'"
          @click="toggleLike"
        >
          👍 点赞 ({{ postStore.currentPost.likeCount }})
        </el-button>
        <el-button @click="toggleFavorite">
          ⭐ 收藏 ({{ postStore.currentPost.favoriteCount }})
        </el-button>
        <el-button type="info">
          💬 评论 ({{ postStore.currentPost.commentCount }})
        </el-button>
      </div>

      <div class="comments-section">
        <h2>评论</h2>
        <div class="comment-form">
          <el-input
            v-model="commentText"
            type="textarea"
            placeholder="发表您的评论..."
            :rows="3"
          />
          <el-button type="primary" @click="postComment">提交评论</el-button>
        </div>
        <div class="comments-list">
          <div class="no-comments">暂无评论，来发表第一条评论吧！</div>
        </div>
      </div>
    </div>
    <el-empty v-else description="内容不存在" />
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { ElMessage } from 'element-plus'
import { usePostStore } from '@/stores/post'

const route = useRoute()
const postStore = usePostStore()

const isLiked = ref(false)
const commentText = ref('')

const postId = Number(route.params.id)

const formatDate = (date: string) => {
  return new Date(date).toLocaleDateString('zh-CN')
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

const toggleLike = async () => {
  try {
    await postStore.likePost(postId)
    isLiked.value = !isLiked.value
    if (postStore.currentPost) {
      postStore.currentPost.likeCount += isLiked.value ? 1 : -1
    }
  } catch (error) {
    ElMessage.error('操作失败')
  }
}

const toggleFavorite = async () => {
  try {
    await postStore.favoritePost(postId)
    ElMessage.success('已收藏')
  } catch (error) {
    ElMessage.error('操作失败')
  }
}

const postComment = () => {
  if (!commentText.value.trim()) {
    ElMessage.warning('请输入评论内容')
    return
  }
  ElMessage.success('评论发表成功')
  commentText.value = ''
}

onMounted(async () => {
  await postStore.fetchPostDetail(postId)
})
</script>

<style scoped>
.post-detail-container {
  max-width: 800px;
  margin: 0 auto;
  padding: 20px;
}

.post-detail {
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  padding: 40px;
}

.post-header {
  border-bottom: 2px solid #eee;
  padding-bottom: 20px;
  margin-bottom: 20px;
}

.post-header h1 {
  margin: 0 0 15px 0;
  font-size: 32px;
  color: #333;
}

.post-meta {
  display: flex;
  gap: 15px;
  font-size: 14px;
  color: #999;
}

.post-body {
  font-size: 16px;
  line-height: 1.8;
  color: #555;
  margin-bottom: 20px;
  white-space: pre-wrap;
  word-break: break-word;
}

.post-tags {
  margin-bottom: 20px;
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.post-actions {
  display: flex;
  gap: 10px;
  margin-bottom: 30px;
  padding: 20px 0;
  border-top: 1px solid #eee;
  border-bottom: 1px solid #eee;
}

.comments-section {
  margin-top: 30px;
}

.comments-section h2 {
  font-size: 20px;
  margin-bottom: 20px;
  color: #333;
}

.comment-form {
  margin-bottom: 20px;
  display: flex;
  gap: 10px;
}

.comment-form :deep(.el-textarea) {
  flex: 1;
}

.comments-list {
  padding: 20px;
  background: #f9f9f9;
  border-radius: 6px;
  min-height: 100px;
}

.no-comments {
  text-align: center;
  color: #999;
  padding: 20px;
}
</style>
