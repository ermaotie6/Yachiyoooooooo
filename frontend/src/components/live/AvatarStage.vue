<template>
  <section class="avatar-section">
    <div class="avatar-wrapper">
      <Live2DComponent
        ref="live2dComponent"
        :width="800"
        :height="600"
        @animation-complete="$emit('animationComplete')"
      />
    </div>

    <!-- 连接状态指示器 -->
    <div class="status-indicator">
      <span :class="['status-dot', connectionStatus]"></span>
      <span class="status-text">
        {{ connectionStatus === 'connected' ? '已连接' : '已断开' }}
      </span>
    </div>

    <!-- 虚拟形象信息 -->
    <div class="avatar-info">
      <h2>Yachiyo</h2>
      <p class="role">AI 虚拟直播助手</p>
    </div>

    <!-- 字幕覆盖层 -->
    <div v-if="subtitleText" class="subtitle-overlay">
      <span class="subtitle-text">{{ subtitleText }}</span>
    </div>
  </section>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import Live2DComponent from '@/components/Live2DComponent.vue'

defineProps<{
  connectionStatus: 'connected' | 'disconnected'
  subtitleText: string
}>()

defineEmits<{
  animationComplete: []
}>()

const live2dComponent = ref<InstanceType<typeof Live2DComponent> | null>(null)

// 暴露 Live2DComponent 的方法给父组件调用
defineExpose({
  get live2d() { return live2dComponent.value }
})
</script>

<style scoped>
.avatar-section {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background: rgba(255, 255, 255, 0.1);
  border-radius: 15px;
  backdrop-filter: blur(10px);
  position: relative;
  overflow: hidden;
}

.avatar-wrapper {
  flex: 1;
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  max-height: 80%;
}

.status-indicator {
  position: absolute;
  top: 20px;
  right: 20px;
  display: flex;
  align-items: center;
  gap: 10px;
  color: white;
  font-size: 14px;
  font-weight: 500;
  background: rgba(0, 0, 0, 0.3);
  padding: 8px 12px;
  border-radius: 20px;
  backdrop-filter: blur(5px);
}

.status-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
}

.status-dot.connected {
  background: #4caf50;
  box-shadow: 0 0 10px #4caf50;
}

.status-dot.disconnected {
  background: #f44336;
  box-shadow: 0 0 10px #f44336;
}

.status-text {
  font-size: 14px;
}

.avatar-info {
  position: absolute;
  bottom: 60px;
  left: 0;
  right: 0;
  text-align: center;
  color: white;
  text-shadow: 0 2px 8px rgba(0, 0, 0, 0.5);
}

.avatar-info h2 {
  margin: 0;
  font-size: 28px;
  font-weight: bold;
}

.avatar-info .role {
  margin: 4px 0 0;
  font-size: 14px;
  opacity: 0.8;
}

.subtitle-overlay {
  position: absolute;
  bottom: 20px;
  left: 10%;
  right: 10%;
  background: rgba(0, 0, 0, 0.7);
  border-radius: 8px;
  padding: 10px 20px;
  text-align: center;
  backdrop-filter: blur(5px);
}

.subtitle-text {
  color: white;
  font-size: 18px;
  font-weight: 500;
  line-height: 1.5;
}

@media (max-width: 1200px) {
  .avatar-section {
    min-height: 400px;
  }
}
</style>
