import { defineStore } from 'pinia'
import { ref } from 'vue'
import type { Post } from '@/types'
import { api } from '@/api/client'

export const usePostStore = defineStore('post', () => {
  const posts = ref<Post[]>([])
  const currentPost = ref<Post | null>(null)
  const loading = ref(false)

  const fetchPosts = async (page: number = 1, pageSize: number = 10) => {
    loading.value = true
    try {
      const response = await api.get('/posts', {
        params: { page, pageSize }
      })
      posts.value = response.data.data.posts
      return response.data.data
    } finally {
      loading.value = false
    }
  }

  const fetchPostDetail = async (postId: number) => {
    const response = await api.get(`/posts/${postId}`)
    currentPost.value = response.data.data
    return response.data.data
  }

  const createPost = async (title: string, content: string, tags: string[]) => {
    const response = await api.post('/posts', {
      title,
      content,
      tags,
      visibility: 'public'
    })
    return response.data
  }

  const updatePost = async (postId: number, title: string, content: string) => {
    const response = await api.put(`/posts/${postId}`, {
      title,
      content
    })
    return response.data
  }

  const deletePost = async (postId: number) => {
    const response = await api.delete(`/posts/${postId}`)
    return response.data
  }

  const likePost = async (postId: number) => {
    const response = await api.post(`/posts/${postId}/like`)
    return response.data
  }

  const favoritePost = async (postId: number) => {
    const response = await api.post(`/posts/${postId}/favorite`)
    return response.data
  }

  return {
    posts,
    currentPost,
    loading,
    fetchPosts,
    fetchPostDetail,
    createPost,
    updatePost,
    deletePost,
    likePost,
    favoritePost
  }
})
