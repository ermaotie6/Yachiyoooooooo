<template>
  <div ref="containerRef" class="live2d-container">
    <!-- PIXI Canvas 会自动挂载到这里 -->

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
        <button class="retry-btn" @click="retryLoad">重试</button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, watch } from 'vue'
import * as PIXI from 'pixi.js'
import { Live2DModel, MotionPreloadStrategy } from 'pixi-live2d-display'

// ============ 注册 Live2D Ticker ============
// pixi-live2d-display 需要 PIXI.Ticker 来驱动更新
Live2DModel.registerTicker(PIXI.Ticker)

// ============ Props ============

interface Props {
  modelPath?: string
  width?: number
  height?: number
}

const props = withDefaults(defineProps<Props>(), {
  modelPath: '/resources/live2d/yachiyo.model3.json',
  width: 400,
  height: 600
})

// ============ 状态管理 ============

const containerRef = ref<HTMLDivElement | null>(null)
const isModelLoaded = ref(false)
const loadError = ref<string | null>(null)

let app: PIXI.Application | null = null
let model: InstanceType<typeof Live2DModel> | null = null
let resizeObserver: ResizeObserver | null = null

// 嘴部同步
let mouthSyncRAF: number | null = null
let audioSyncRAF: number | null = null
let targetMouthOpenY = 0
let currentMouthOpenY = 0

// ============ 核心方法 ============

/**
 * 初始化 PIXI Application
 */
const initPixiApp = (): boolean => {
  const container = containerRef.value
  if (!container) {
    loadError.value = '容器元素不可用'
    return false
  }

  const rect = container.getBoundingClientRect()
  const width = rect.width || props.width
  const height = rect.height || props.height

  app = new PIXI.Application({
    width,
    height,
    backgroundAlpha: 0, // 透明背景
    antialias: true,
    resolution: window.devicePixelRatio || 1,
    autoDensity: true
  })

  // 将 PIXI canvas 挂载到容器
  container.appendChild(app.view as HTMLCanvasElement)

  console.log('[Live2D] PIXI Application initialized:', width, 'x', height)
  return true
}

/**
 * 加载 Live2D 模型
 */
const loadModel = async () => {
  if (!app) return

  try {
    console.log('[Live2D] Loading model from:', props.modelPath)
    loadError.value = null

    model = await Live2DModel.from(props.modelPath, {
      motionPreload: MotionPreloadStrategy.IDLE,
      autoInteract: false // 我们自己控制交互
    })

    if (!model) {
      throw new Error('模型加载返回 null')
    }

    // 调整模型大小使其适应容器
    fitModelToStage()

    // 添加模型到舞台
    app.stage.addChild(model as unknown as PIXI.DisplayObject)

    isModelLoaded.value = true
    loadError.value = null

    // 启动嘴部同步循环
    startMouthSyncLoop()

    console.log('[Live2D] Model loaded successfully')
  } catch (error) {
    const msg = error instanceof Error ? error.message : String(error)
    loadError.value = msg
    console.error('[Live2D] Error loading model:', error)
  }
}

/**
 * 调整模型大小以适应舞台
 */
const fitModelToStage = () => {
  if (!model || !app) return

  const stageWidth = app.screen.width
  const stageHeight = app.screen.height

  // 计算缩放比例，保持纵横比
  const scaleX = stageWidth / model.width
  const scaleY = stageHeight / model.height
  const scale = Math.min(scaleX, scaleY) * 0.9 // 留一点边距

  model.scale.set(scale)

  // 居中显示
  model.x = (stageWidth - model.width * scale) / 2
  model.y = (stageHeight - model.height * scale) / 2
}

/**
 * 嘴部同步循环 — 平滑过渡 mouthOpenY
 */
const startMouthSyncLoop = () => {
  const tick = () => {
    if (!model) return

    // 平滑插值
    currentMouthOpenY += (targetMouthOpenY - currentMouthOpenY) * 0.3

    // 设置 Live2D 参数
    try {
      const coreModel = (model as any).internalModel?.coreModel
      if (coreModel) {
        // Cubism 4 参数名
        coreModel.setParameterValueById('ParamMouthOpenY', currentMouthOpenY)
      }
    } catch {
      // 静默忽略参数设置错误
    }

    mouthSyncRAF = requestAnimationFrame(tick)
  }
  tick()
}

/**
 * 处理容器大小变化
 */
const handleResize = () => {
  if (!app || !containerRef.value) return

  const rect = containerRef.value.getBoundingClientRect()
  app.renderer.resize(rect.width, rect.height)
  fitModelToStage()
}

/**
 * 重试加载
 */
const retryLoad = () => {
  loadError.value = null
  isModelLoaded.value = false
  if (model && app) {
    app.stage.removeChild(model as unknown as PIXI.DisplayObject)
    model.destroy()
    model = null
  }
  loadModel()
}

// ============ 对外接口 ============

/**
 * 设置表情
 * @param expressionName 表情名称 (例: "f_smile", "f_sad")
 * @param _durationMs 持续时间 (Live2D SDK 自动管理过渡)
 */
const setExpression = (expressionName: string, _durationMs?: number) => {
  if (!model) {
    console.warn('[Live2D] Model not loaded, cannot set expression:', expressionName)
    return
  }

  try {
    model.expression(expressionName)
    console.log('[Live2D] Expression set:', expressionName)
  } catch (error) {
    console.warn('[Live2D] Failed to set expression:', expressionName, error)
  }
}

/**
 * 播放动作
 * @param motionName 动作名/组名 (例: "m_greet", "idle")
 * @param priority 优先级 (0=idle, 1=normal, 2=force)
 */
const playMotion = (motionName: string, priority: number = 1) => {
  if (!model) {
    console.warn('[Live2D] Model not loaded, cannot play motion:', motionName)
    return
  }

  try {
    // pixi-live2d-display 的 motion(group, index, priority)
    // motionName 作为 group 名称
    model.motion(motionName, undefined, priority)
    console.log('[Live2D] Motion played:', motionName, 'priority:', priority)
  } catch (error) {
    console.warn('[Live2D] Failed to play motion:', motionName, error)
  }
}

/**
 * 实时嘴部同步
 * @param value 0~1 的开合度
 */
const setSyncMouthOpenY = (value: number) => {
  targetMouthOpenY = Math.max(0, Math.min(1, value))
}

/**
 * 眼睛跟踪 — 让模型看向某个方向
 * @param x -1 (左) ~ 1 (右)
 * @param y -1 (上) ~ 1 (下)
 */
const setEyeTrackingTarget = (x: number, y: number) => {
  if (!model) return

  try {
    const coreModel = (model as any).internalModel?.coreModel
    if (coreModel) {
      coreModel.setParameterValueById('ParamEyeBallX', Math.max(-1, Math.min(1, x)))
      coreModel.setParameterValueById('ParamEyeBallY', Math.max(-1, Math.min(1, y)))
    }
  } catch {
    // 静默忽略
  }
}

/**
 * 设置眼睑开度
 * @param left 左眼 0~1
 * @param right 右眼 0~1
 */
const setEyeOpen = (left: number, right: number) => {
  if (!model) return

  try {
    const coreModel = (model as any).internalModel?.coreModel
    if (coreModel) {
      coreModel.setParameterValueById('ParamEyeLOpen', Math.max(0, Math.min(1, left)))
      coreModel.setParameterValueById('ParamEyeROpen', Math.max(0, Math.min(1, right)))
    }
  } catch {
    // 静默忽略
  }
}

/**
 * 从音频数据更新口型（用于 TTS 播放时的口型同步）
 * @param analyserNode Web Audio API AnalyserNode
 */
const syncMouthFromAudio = (analyserNode: AnalyserNode) => {
  const dataArray = new Uint8Array(analyserNode.fftSize)

  const update = () => {
    analyserNode.getByteTimeDomainData(dataArray)

    // 计算音量 RMS
    let sum = 0
    for (let i = 0; i < dataArray.length; i++) {
      const val = (dataArray[i] - 128) / 128
      sum += val * val
    }
    const rms = Math.sqrt(sum / dataArray.length)

    // 映射到嘴部开合 (0~1)
    setSyncMouthOpenY(Math.min(1, rms * 4))

    audioSyncRAF = requestAnimationFrame(update)
  }
  update()
}

// ============ 生命周期 ============

onMounted(async () => {
  if (!initPixiApp()) return

  // 监听容器大小变化
  if (containerRef.value) {
    resizeObserver = new ResizeObserver(handleResize)
    resizeObserver.observe(containerRef.value)
  }

  await loadModel()
})

onUnmounted(() => {
  // 停止嘴部同步
  if (mouthSyncRAF !== null) {
    cancelAnimationFrame(mouthSyncRAF)
    mouthSyncRAF = null
  }

  // 停止音频同步
  if (audioSyncRAF !== null) {
    cancelAnimationFrame(audioSyncRAF)
    audioSyncRAF = null
  }

  // 断开 ResizeObserver
  if (resizeObserver) {
    resizeObserver.disconnect()
    resizeObserver = null
  }

  // 销毁模型
  if (model) {
    model.destroy()
    model = null
  }

  // 销毁 PIXI 应用
  if (app) {
    app.destroy(true, { children: true, texture: true, baseTexture: true })
    app = null
  }
})

// ============ 暴露方法 ============

defineExpose({
  setExpression,
  playMotion,
  setSyncMouthOpenY,
  setEyeTrackingTarget,
  setEyeOpen,
  syncMouthFromAudio,
  retryLoad
})
</script>

<style scoped>
.live2d-container {
  position: relative;
  width: 100%;
  height: 100%;
  background: transparent;
  overflow: hidden;
}

/* PIXI 自动创建的 canvas */
.live2d-container :deep(canvas) {
  width: 100% !important;
  height: 100% !important;
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
  text-align: center;
  max-width: 300px;
}

.retry-btn {
  padding: 8px 20px;
  border: 1px solid rgba(255, 255, 255, 0.5);
  border-radius: 6px;
  background: rgba(255, 255, 255, 0.1);
  color: white;
  cursor: pointer;
  font-size: 14px;
  transition: background 0.2s;
}

.retry-btn:hover {
  background: rgba(255, 255, 255, 0.25);
}
</style>
