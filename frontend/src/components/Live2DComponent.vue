<template>
  <div class="live2d-container">
    <canvas ref="canvasRef" class="live2d-canvas"></canvas>

    <!-- 模型加载提示 -->
    <div v-if="!isModelLoaded" class="loading-overlay">
      <div class="loading-content">
        <div class="spinner"></div>
        <p>加载虚拟形象中...</p>
      </div>
    </div>

    <!-- 错误提示 -->
    <div v-if="loadError" class="error-overlay">
      <div class="error-content">
        <p>⚠️ 虚拟形象加载失败</p>
        <small>{{ loadError }}</small>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'

// ============ 状态管理 ============

const canvasRef = ref<HTMLCanvasElement | null>(null)
const isModelLoaded = ref(false)
const loadError = ref<string | null>(null)

// 模型状态
let canvas: HTMLCanvasElement | null = null
let ctx: CanvasRenderingContext2D | null = null

// 参数状态
const modelState = {
  mouthOpenY: 0,
  eyeOpenLeft: 1.0,
  eyeOpenRight: 1.0,
  eyeX: 0,
  eyeY: 0,
  currentExpression: 'neutral',
  expressionBlend: 0,
  targetExpression: 'neutral'
}

// 动画队列
const animationQueue: any[] = []
let currentAnimation: any = null
let animationStartTime = 0

// ============ 核心方法 ============

/**
 * 初始化 Canvas
 */
const initCanvas = () => {
  canvas = canvasRef.value
  if (!canvas) {
    loadError.value = 'Canvas 不可用'
    return false
  }

  ctx = canvas.getContext('2d')
  if (!ctx) {
    loadError.value = '获取 Canvas 上下文失败'
    return false
  }

  // 设置 Canvas 大小
  const rect = canvas.getBoundingClientRect()
  canvas.width = rect.width * window.devicePixelRatio
  canvas.height = rect.height * window.devicePixelRatio
  ctx.scale(window.devicePixelRatio, window.devicePixelRatio)

  console.log('[Live2D] Canvas initialized:', canvas.width, canvas.height)
  return true
}

/**
 * 加载模型
 */
const loadModel = async () => {
  try {
    // 注: 实际部署时，这里会加载真实的 Live2D 模型
    // 目前使用模拟渲染

    console.log('[Live2D] Model loading started')

    // 模拟加载延迟
    await new Promise((resolve) => setTimeout(resolve, 500))

    isModelLoaded.value = true
    loadError.value = null
    console.log('[Live2D] Model loaded successfully')

    // 启动渲染循环
    startRenderLoop()
  } catch (error) {
    loadError.value = String(error)
    console.error('[Live2D] Error loading model:', error)
  }
}

/**
 * 渲染循环
 */
const startRenderLoop = () => {
  const render = () => {
    updateAnimation()
    draw()
    requestAnimationFrame(render)
  }
  render()
}

/**
 * 更新动画状态
 */
const updateAnimation = () => {
  if (!currentAnimation) {
    if (animationQueue.length > 0) {
      currentAnimation = animationQueue.shift()
      animationStartTime = Date.now()
    }
    return
  }

  const elapsed = Date.now() - animationStartTime
  const duration = currentAnimation.duration || 1000
  const progress = Math.min(elapsed / duration, 1)

  if (currentAnimation.type === 'expression') {
    modelState.expressionBlend = progress

    if (progress >= 1) {
      modelState.currentExpression = currentAnimation.expression
      modelState.expressionBlend = 1
      currentAnimation = null
    }
  } else if (currentAnimation.type === 'motion') {
    // 动作播放逻辑
    if (progress >= 1) {
      currentAnimation = null
    }
  }

  // 平滑过渡嘴部开合
  const targetMouth = modelState.mouthOpenY
  const currentMouth = modelState.mouthOpenY
  modelState.mouthOpenY += (targetMouth - currentMouth) * 0.2
}

/**
 * 绘制模型
 */
const draw = () => {
  if (!canvas || !ctx) return

  const width = canvas.width / window.devicePixelRatio
  const height = canvas.height / window.devicePixelRatio

  // 清空画布
  ctx.fillStyle = 'transparent'
  ctx.clearRect(0, 0, width, height)

  // 绘制头部 (圆形)
  const headX = width / 2
  const headY = height / 2.2
  const headRadius = 80

  // 头部阴影
  ctx.fillStyle = 'rgba(0, 0, 0, 0.1)'
  ctx.beginPath()
  ctx.arc(headX, headY + 5, headRadius, 0, Math.PI * 2)
  ctx.fill()

  // 头部主体
  const headGradient = ctx.createRadialGradient(
    headX - 20,
    headY - 30,
    10,
    headX,
    headY,
    headRadius
  )
  headGradient.addColorStop(0, '#fdbcb4')
  headGradient.addColorStop(1, '#f5a89f')
  ctx.fillStyle = headGradient
  ctx.beginPath()
  ctx.arc(headX, headY, headRadius, 0, Math.PI * 2)
  ctx.fill()

  // 绘制眼睛
  drawEyes(headX, headY, headRadius)

  // 绘制嘴巴
  drawMouth(headX, headY + 30, headRadius)

  // 绘制头发
  drawHair(headX, headY, headRadius)

  // 绘制身体
  drawBody(headX, headY + headRadius + 10)

  // 绘制表情覆盖
  drawExpression(headX, headY, headRadius)
}

/**
 * 绘制眼睛
 */
const drawEyes = (centerX: number, centerY: number, headRadius: number) => {
  if (!ctx) return

  const eyeY = centerY - 20
  const eyeLeftX = centerX - 25
  const eyeRightX = centerX + 25

  // 眼睛白部
  ctx.fillStyle = 'white'
  ctx.beginPath()
  ctx.arc(eyeLeftX, eyeY, 12, 0, Math.PI * 2)
  ctx.fill()
  ctx.beginPath()
  ctx.arc(eyeRightX, eyeY, 12, 0, Math.PI * 2)
  ctx.fill()

  // 瞳孔 (跟踪)
  const pupilOffset = 6
  const pupilLeftX = eyeLeftX + modelState.eyeX * pupilOffset
  const pupilLeftY = eyeY + modelState.eyeY * pupilOffset
  const pupilRightX = eyeRightX + modelState.eyeX * pupilOffset
  const pupilRightY = eyeY + modelState.eyeY * pupilOffset

  ctx.fillStyle = '#333'
  ctx.beginPath()
  ctx.arc(pupilLeftX, pupilLeftY, 7, 0, Math.PI * 2)
  ctx.fill()
  ctx.beginPath()
  ctx.arc(pupilRightX, pupilRightY, 7, 0, Math.PI * 2)
  ctx.fill()

  // 眼神光
  ctx.fillStyle = 'rgba(255, 255, 255, 0.6)'
  ctx.beginPath()
  ctx.arc(pupilLeftX - 2, pupilLeftY - 2, 3, 0, Math.PI * 2)
  ctx.fill()
  ctx.beginPath()
  ctx.arc(pupilRightX - 2, pupilRightY - 2, 3, 0, Math.PI * 2)
  ctx.fill()

  // 眼睑
  if (modelState.eyeOpenLeft < 1) {
    ctx.fillStyle = '#fdbcb4'
    ctx.beginPath()
    ctx.arc(
      eyeLeftX,
      eyeY - 12,
      12,
      0,
      Math.PI * 2
    )
    ctx.fill()
  }
}

/**
 * 绘制嘴巴
 */
const drawMouth = (centerX: number, centerY: number, headRadius: number) => {
  if (!ctx) return

  const mouthWidth = 15
  const mouthHeight = 5 + modelState.mouthOpenY * 15

  // 嘴唇
  ctx.strokeStyle = '#c1686e'
  ctx.lineWidth = 2
  ctx.beginPath()
  ctx.ellipse(centerX, centerY, mouthWidth, mouthHeight, 0, 0, Math.PI * 2)
  ctx.stroke()

  // 嘴唇填充 (轻微)
  ctx.fillStyle = 'rgba(193, 104, 110, 0.2)'
  ctx.beginPath()
  ctx.ellipse(centerX, centerY, mouthWidth, mouthHeight, 0, 0, Math.PI * 2)
  ctx.fill()
}

/**
 * 绘制头发
 */
const drawHair = (centerX: number, centerY: number, headRadius: number) => {
  if (!ctx) return

  ctx.fillStyle = '#8B4513'

  // 前发
  ctx.beginPath()
  ctx.ellipse(centerX, centerY - headRadius + 20, headRadius - 10, 30, 0, 0, Math.PI)
  ctx.fill()

  // 侧发
  ctx.beginPath()
  ctx.moveTo(centerX - headRadius, centerY - 20)
  ctx.bezierCurveTo(
    centerX - headRadius - 20,
    centerY - 40,
    centerX - headRadius - 30,
    centerY,
    centerX - headRadius - 10,
    centerY + 30
  )
  ctx.bezierCurveTo(
    centerX - headRadius,
    centerY,
    centerX - headRadius + 10,
    centerY - 10,
    centerX - headRadius,
    centerY - 20
  )
  ctx.fill()

  // 右侧发
  ctx.beginPath()
  ctx.moveTo(centerX + headRadius, centerY - 20)
  ctx.bezierCurveTo(
    centerX + headRadius + 20,
    centerY - 40,
    centerX + headRadius + 30,
    centerY,
    centerX + headRadius + 10,
    centerY + 30
  )
  ctx.bezierCurveTo(
    centerX + headRadius,
    centerY,
    centerX + headRadius - 10,
    centerY - 10,
    centerX + headRadius,
    centerY - 20
  )
  ctx.fill()
}

/**
 * 绘制身体
 */
const drawBody = (centerX: number, topY: number) => {
  if (!ctx) return

  // 颈部
  ctx.fillStyle = '#fdbcb4'
  ctx.beginPath()
  ctx.rect(centerX - 15, topY, 30, 30)
  ctx.fill()

  // 衣服
  ctx.fillStyle = '#667eea'
  ctx.beginPath()
  ctx.moveTo(centerX - 40, topY + 30)
  ctx.lineTo(centerX + 40, topY + 30)
  ctx.lineTo(centerX + 45, topY + 80)
  ctx.lineTo(centerX - 45, topY + 80)
  ctx.closePath()
  ctx.fill()

  // 衣服装饰
  ctx.strokeStyle = '#764ba2'
  ctx.lineWidth = 1
  ctx.beginPath()
  ctx.moveTo(centerX - 40, topY + 30)
  ctx.lineTo(centerX + 40, topY + 30)
  ctx.stroke()
}

/**
 * 绘制表情覆盖
 */
const drawExpression = (centerX: number, centerY: number, headRadius: number) => {
  if (!ctx || modelState.expressionBlend === 0) return

  const blend = modelState.expressionBlend

  switch (modelState.targetExpression) {
    case 'happy':
      // 开心表情 - 眼睛和嘴巴的调整由其他函数处理
      break
    case 'sad':
      // 伤心表情
      ctx.strokeStyle = `rgba(200, 100, 100, ${blend * 0.3})`
      ctx.lineWidth = 2
      // 泪水
      ctx.beginPath()
      ctx.arc(centerX - 25, centerY - 15, 4, 0, Math.PI * 2)
      ctx.stroke()
      break
    case 'surprised':
      // 惊讶表情
      break
  }
}

// ============ 对外接口 ============

/**
 * 设置表情
 */
const setExpression = (expressionName: string, durationMs: number) => {
  console.log('[Live2D] Set expression:', expressionName, durationMs)

  animationQueue.push({
    type: 'expression',
    expression: expressionName,
    duration: durationMs
  })
}

/**
 * 播放动作
 */
const playMotion = (motionName: string, priority: number = 0) => {
  console.log('[Live2D] Play motion:', motionName, 'priority:', priority)

  animationQueue.push({
    type: 'motion',
    motion: motionName,
    duration: 1000,
    priority
  })
}

/**
 * 实时嘴部同步
 */
const setSyncMouthOpenY = (value: number) => {
  modelState.mouthOpenY = Math.max(0, Math.min(1, value))
}

/**
 * 眼睛跟踪
 */
const setEyeTrackingTarget = (x: number, y: number) => {
  modelState.eyeX = Math.max(-1, Math.min(1, x))
  modelState.eyeY = Math.max(-1, Math.min(1, y))
}

/**
 * 设置眼睑开度
 */
const setEyeOpen = (left: number, right: number) => {
  modelState.eyeOpenLeft = Math.max(0, Math.min(1, left))
  modelState.eyeOpenRight = Math.max(0, Math.min(1, right))
}

// ============ 生命周期 ============

onMounted(async () => {
  if (!initCanvas()) {
    loadError.value = '初始化 Canvas 失败'
    return
  }

  await loadModel()
})

onUnmounted(() => {
  // 清理资源
  canvas = null
  ctx = null
})

// ============ 暴露方法 ============

defineExpose({
  setExpression,
  playMotion,
  setSyncMouthOpenY,
  setEyeTrackingTarget,
  setEyeOpen
})
</script>

<style scoped>
.live2d-container {
  position: relative;
  width: 100%;
  height: 100%;
  background: transparent;
}

.live2d-canvas {
  width: 100%;
  height: 100%;
  display: block;
}

.loading-overlay,
.error-overlay {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(0, 0, 0, 0.5);
  backdrop-filter: blur(5px);
  z-index: 10;
}

.loading-content,
.error-content {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 15px;
  color: white;
}

.spinner {
  width: 40px;
  height: 40px;
  border: 4px solid rgba(255, 255, 255, 0.3);
  border-top-color: white;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

.loading-content p,
.error-content p {
  font-size: 16px;
  font-weight: 500;
  margin: 0;
}

.error-content small {
  font-size: 12px;
  opacity: 0.8;
}
</style>
