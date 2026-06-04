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
  background: var(--c-surface);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-lg);
  position: relative;
  box-shadow: var(--shadow-md);
  backdrop-filter: blur(20px);
  min-height: 0;
  max-height: 100%;
  overflow: hidden;
}

.avatar-wrapper {
  flex: 1;
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
}

/* 状态指示器 — 右上胶囊 */
.status-indicator {
  position: absolute;
  top: 12px;
  right: 12px;
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 12px;
  font-weight: 500;
  background: rgba(0,0,0,0.5);
  padding: 6px 14px;
  border-radius: 20px;
  backdrop-filter: blur(10px);
  border: 1px solid var(--c-border);
  z-index: 5;
}
.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  transition: all var(--dur-normal);
}
.status-dot.connected {
  background: var(--c-success);
  box-shadow: 0 0 8px rgba(0,206,201,0.5);
  animation: pulse-glow 2s infinite;
}
.status-dot.disconnected {
  background: var(--c-error);
  box-shadow: 0 0 8px rgba(255,118,117,0.3);
}
.status-text { color: var(--c-text); opacity: 0.85; }

/* 字幕 — 底部渐变条 */
.subtitle-overlay {
  position: absolute;
  bottom: 24px;
  left: 8%;
  right: 8%;
  background: linear-gradient(90deg, rgba(0,0,0,0) 0%, rgba(0,0,0,0.75) 20%, rgba(0,0,0,0.75) 80%, rgba(0,0,0,0) 100%);
  border-radius: var(--radius-sm);
  padding: 12px 24px;
  text-align: center;
  backdrop-filter: blur(8px);
  z-index: 5;
}
.subtitle-text {
  color: #fff;
  font-size: 17px;
  font-weight: 500;
  line-height: 1.5;
  text-shadow: 0 1px 4px rgba(0,0,0,0.5);
  letter-spacing: 0.5px;
}

@media (max-width: 1100px) {
  .avatar-section { min-height: 50vh; }
  .subtitle-text { font-size: 15px; }
}
</style>
