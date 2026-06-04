<template>
  <div ref="containerRef" class="live2d-container">
    <div v-if="!isModelLoaded" class="loading-overlay">
      <div class="loading-content">
        <div class="spinner"></div>
        <p>加载虚拟形象中...</p>
      </div>
    </div>
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
import { ref, onMounted, onUnmounted } from 'vue'
import * as PIXI from 'pixi.js'
import { Live2DModel, MotionPreloadStrategy } from 'pixi-live2d-display/cubism4'

Live2DModel.registerTicker(PIXI.Ticker)

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

// ============ 空闲动画 ============
let idleRAF: number | null = null
let idleTime = 0
let lastIdleFrame = 0

// 眨眼
let nextBlinkAt = 3000
let blinkStartTime = 0
let isBlinking = false
const BLINK_DURATION = 150
const BLINK_HOLD = 30
const BLINK_RECOVER = 100
const BLINK_TOTAL = BLINK_DURATION + BLINK_HOLD + BLINK_RECOVER
const BLINK_INTERVAL_MIN = 2500
const BLINK_INTERVAL_MAX = 6000

// 参数动画（动作降级：无需 .motion3.json 文件）
interface ParamTween {
  paramId: string
  baseValue: number
  amplitude: number
  frequency: number
  phaseOffset: number
}
let activeParamTweens: ParamTween[] = []
let paramTweenStartTime = 0
let paramTweenDuration = 0
let paramTweenPriority = 0

// ============ 工具函数 ============
const easeInOutSine = (t: number): number => -(Math.cos(Math.PI * t) - 1) / 2
const easeOutCubic = (t: number): number => 1 - Math.pow(1 - t, 3)
const easeInCubic = (t: number): number => t * t * t

const setParam = (paramId: string, value: number): void => {
  if (!model) return
  try {
    const coreModel = (model as any).internalModel?.coreModel
    if (coreModel) coreModel.setParameterValueById(paramId, value)
  } catch { /* ignore */ }
}

const setParams = (entries: [string, number][]): void => {
  if (!model) return
  try {
    const coreModel = (model as any).internalModel?.coreModel
    if (coreModel) for (const [id, val] of entries) coreModel.setParameterValueById(id, val)
  } catch { /* ignore */ }
}

// ============ 空闲动画循环 ============
const startIdleLoop = () => {
  if (idleRAF !== null) return

  const tick = (timestamp: number) => {
    if (!model) { idleRAF = requestAnimationFrame(tick); return }

    if (lastIdleFrame === 0) lastIdleFrame = timestamp
    const dt = timestamp - lastIdleFrame
    lastIdleFrame = timestamp
    idleTime += dt

    // 眨眼
    if (isBlinking) {
      const elapsed = timestamp - blinkStartTime
      if (elapsed < BLINK_DURATION) {
        const t = elapsed / BLINK_DURATION
        setParams([['ParamEyeLOpen', 1 - easeInCubic(t)], ['ParamEyeROpen', 1 - easeInCubic(t)]])
      } else if (elapsed < BLINK_DURATION + BLINK_HOLD) {
        setParams([['ParamEyeLOpen', 0], ['ParamEyeROpen', 0]])
      } else if (elapsed < BLINK_TOTAL) {
        const t = (elapsed - BLINK_DURATION - BLINK_HOLD) / BLINK_RECOVER
        setParams([['ParamEyeLOpen', easeOutCubic(t)], ['ParamEyeROpen', easeOutCubic(t)]])
      } else {
        isBlinking = false
        setParams([['ParamEyeLOpen', 1], ['ParamEyeROpen', 1]])
        nextBlinkAt = idleTime + BLINK_INTERVAL_MIN + Math.random() * (BLINK_INTERVAL_MAX - BLINK_INTERVAL_MIN)
      }
    } else if (idleTime >= nextBlinkAt && activeParamTweens.length === 0) {
      isBlinking = true
      blinkStartTime = timestamp
    }

    // 呼吸 — 身体轻微前后
    if (activeParamTweens.length === 0 || paramTweenPriority < 2) {
      setParam('ParamAngle_BodyZ', Math.sin(idleTime * 0.0012) * 0.03)
    }

    // 头部微晃
    if (activeParamTweens.length === 0) {
      setParams([
        ['ParamAngleX', Math.sin(idleTime * 0.0005 + 0.7) * 0.04],
        ['ParamAngleY', Math.cos(idleTime * 0.0007 + 1.3) * 0.03],
        ['ParamAngleZ', Math.sin(idleTime * 0.0006) * 0.02]
      ])
    }

    // 参数动画（动作降级）
    if (activeParamTweens.length > 0) {
      const elapsed = timestamp - paramTweenStartTime
      const progress = Math.min(1, elapsed / paramTweenDuration)
      const eased = easeInOutSine(progress)
      const decay = progress > 0.7 ? 1 - ((progress - 0.7) / 0.3) : 1

      for (const tween of activeParamTweens) {
        const osc = Math.sin(eased * tween.frequency * Math.PI * 2 * paramTweenDuration / 1000 + tween.phaseOffset)
        setParam(tween.paramId, tween.baseValue + tween.amplitude * osc * decay)
      }

      if (progress >= 1) {
        for (const tween of activeParamTweens) setParam(tween.paramId, tween.baseValue)
        activeParamTweens = []
        paramTweenStartTime = 0
        paramTweenDuration = 0
        paramTweenPriority = 0
      }
    }

    idleRAF = requestAnimationFrame(tick)
  }
  idleRAF = requestAnimationFrame(tick)
  console.log('[Live2D] Idle loop started (blink + breathe + sway)')
}

const stopIdleLoop = () => {
  if (idleRAF !== null) { cancelAnimationFrame(idleRAF); idleRAF = null }
  lastIdleFrame = 0
}

// ============ 动作映射（参数降级方案） ============
// 每个动作定义一组参数的振荡配置。模型无需 .motion3.json 也能动。
// 同时支持 "Tap Body"（后端旧名）和 "TapBody"（模型真实组名）
const MOTION_PARAM_MAP: Record<string, (duration: number) => ParamTween[]> = {
  'Tap Body': () => [
    { paramId: 'ParamAngle_BodyX', baseValue: 0, amplitude: 0.25, frequency: 2.5, phaseOffset: 0 },
    { paramId: 'ParamAngle_BodyY', baseValue: 0, amplitude: 0.12, frequency: 2.5, phaseOffset: Math.PI / 4 }
  ],
  'TapBody': () => [
    { paramId: 'ParamAngle_BodyX', baseValue: 0, amplitude: 0.25, frequency: 2.5, phaseOffset: 0 },
    { paramId: 'ParamAngle_BodyY', baseValue: 0, amplitude: 0.12, frequency: 2.5, phaseOffset: Math.PI / 4 }
  ],
  'Nod': () => [
    { paramId: 'ParamAngleX', baseValue: 0, amplitude: 0.18, frequency: 2.0, phaseOffset: 0 }
  ],
  'Shake': () => [
    { paramId: 'ParamAngleY', baseValue: 0, amplitude: 0.22, frequency: 3.0, phaseOffset: 0 }
  ],
  'Think': () => [
    { paramId: 'ParamAngleZ', baseValue: 0.08, amplitude: 0.04, frequency: 1.2, phaseOffset: 0 },
    { paramId: 'ParamAngleX', baseValue: -0.05, amplitude: 0.03, frequency: 0.8, phaseOffset: Math.PI / 3 }
  ],
  'Idle': () => []
}

// ============ PIXI 初始化 ============
const initPixiApp = (): boolean => {
  const container = containerRef.value
  if (!container) { loadError.value = '容器元素不可用'; return false }
  const rect = container.getBoundingClientRect()
  const w = rect.width || props.width
  const h = rect.height || props.height
  app = new PIXI.Application({
    width: w,
    height: h,
    backgroundAlpha: 0,
    antialias: true,
    resolution: Math.min(window.devicePixelRatio, 2),
    autoDensity: true
  })
  container.appendChild(app.view as HTMLCanvasElement)
  // 容器大小变化时同步 PIXI 渲染器
  resizeObserver = new ResizeObserver(() => {
    if (!app || !containerRef.value) return
    const r = containerRef.value.getBoundingClientRect()
    if (r.width > 0 && r.height > 0) {
      app.renderer.resize(r.width, r.height)
      fitModelToStage()
    }
  })
  resizeObserver.observe(container)
  return true
}

const loadModel = async () => {
  if (!app) return
  try {
    model = await Live2DModel.from(props.modelPath, {
      motionPreload: MotionPreloadStrategy.IDLE,
      autoInteract: false
    })
    if (!model) throw new Error('模型加载返回 null')
    fitModelToStage()
    app.stage.addChild(model as unknown as PIXI.DisplayObject)
    isModelLoaded.value = true
    loadError.value = null
    startMouthSyncLoop()
    startIdleLoop()
    console.log('[Live2D] Model loaded + idle started')
  } catch (error) {
    loadError.value = error instanceof Error ? error.message : String(error)
  }
}

const fitModelToStage = () => {
  if (!model || !app) return
  const m = model as any
  const sw = app.screen.width
  const sh = app.screen.height
  const mw = m.internalModel?.width || m.width || 2000
  const mh = m.internalModel?.height || m.height || 2000
  const scale = Math.min(sw / mw, sh / mh) * 1.5
  model.scale.set(scale)
  // 居中对齐（左上角锚点），带微调偏移
  model.x = (sw - mw * scale) / 2 - sw * 0.18
  model.y = (sh - mh * scale) / 2 + sh * 0.22
  console.log('[Live2D] fitModel: screen=(' + sw + ',' + sh + ') model=(' + mw + ',' + mh + ') scale=' + scale.toFixed(3) + ' pos=(' + model.x.toFixed(0) + ',' + model.y.toFixed(0) + ')')
}

const startMouthSyncLoop = () => {
  const tick = () => {
    if (!model) return
    currentMouthOpenY += (targetMouthOpenY - currentMouthOpenY) * 0.3
    setParam('ParamMouthOpenY', currentMouthOpenY)
    mouthSyncRAF = requestAnimationFrame(tick)
  }
  tick()
}

const handleResize = () => {
  if (!app || !containerRef.value) return
  app.renderer.resize(containerRef.value.getBoundingClientRect().width, containerRef.value.getBoundingClientRect().height)
  fitModelToStage()
}

const retryLoad = () => {
  loadError.value = null; isModelLoaded.value = false; stopIdleLoop()
  if (model && app) { app.stage.removeChild(model as unknown as PIXI.DisplayObject); model.destroy(); model = null }
  loadModel()
}

// ============ 对外接口 ============

const setExpression = (expressionName: string, _durationMs?: number) => {
  if (!model) { console.warn('[Live2D] setExpression: model not loaded'); return }
  try {
    model.expression(expressionName)
    console.log('[Live2D] Expression set:', expressionName)
  } catch (e) {
    console.warn('[Live2D] Expression failed:', expressionName, e)
  }
}

/**
 * 播放动作（无需 .motion3.json 的降级方案）
 *
 * 优先尝试原生 motion API（如果将来有 motion3.json 则自动生效），
 * 没有 motion 文件时降级为直接操作 Live2D 参数模拟动作效果。
 *
 * 支持的动作名: "TapBody"|"Tap Body"(挥手) | "Nod"(点头) | "Shake"(摇头) | "Think"(思考) | "Idle"(恢复默认)
 */
const playMotion = (motionName: string, priority: number = 1) => {
  if (!model) return
  if (activeParamTweens.length > 0 && priority <= paramTweenPriority) return

  // 规范化组名: 后端发 "Tap Body"，模型是 "TapBody"
  const normalized = motionName === 'Tap Body' ? 'TapBody' : motionName

  // 先尝试原生 motion
  try {
    model.motion(normalized, undefined, priority)
    console.log('[Live2D] Native motion played:', normalized)
    return
  } catch { /* 降级到参数动画 */ }

  const mapper = MOTION_PARAM_MAP[motionName] || MOTION_PARAM_MAP[normalized]
  if (!mapper) {
    console.warn('[Live2D] Unknown motion:', motionName)
    return
  }

  if (motionName === 'Idle') {
    activeParamTweens = []; paramTweenStartTime = 0; paramTweenDuration = 0; paramTweenPriority = 0
    return
  }

  let duration = 2000
  if (motionName === 'Nod') duration = 1800
  else if (motionName === 'Shake') duration = 1500
  else if (motionName === 'Think') duration = 2500

  const tweens = mapper(duration)
  if (!tweens.length) return

  activeParamTweens = tweens
  paramTweenStartTime = performance.now()
  paramTweenDuration = duration
  paramTweenPriority = priority
  console.log('[Live2D] Motion (param fallback):', motionName, 'duration:', duration)
}

const setSyncMouthOpenY = (value: number) => { targetMouthOpenY = Math.max(0, Math.min(1, value)) }

const setEyeTrackingTarget = (x: number, y: number) => {
  if (!model) return
  setParams([['ParamEyeBallX', Math.max(-1, Math.min(1, x))], ['ParamEyeBallY', Math.max(-1, Math.min(1, y))]])
}

const setEyeOpen = (left: number, right: number) => {
  if (!model) return
  setParams([['ParamEyeLOpen', Math.max(0, Math.min(1, left))], ['ParamEyeROpen', Math.max(0, Math.min(1, right))]])
}

const syncMouthFromAudio = (analyserNode: AnalyserNode) => {
  const dataArray = new Uint8Array(analyserNode.fftSize)
  const update = () => {
    analyserNode.getByteTimeDomainData(dataArray)
    let sum = 0
    for (let i = 0; i < dataArray.length; i++) { const v = (dataArray[i] - 128) / 128; sum += v * v }
    setSyncMouthOpenY(Math.min(1, Math.sqrt(sum / dataArray.length) * 4))
    audioSyncRAF = requestAnimationFrame(update)
  }
  update()
}

// ============ 可见性管理 (省 CPU — 标签页切后台时暂停渲染) ============
let pageVisible = true
let pausedByVisibility = false

const pauseRendering = () => {
  if (pausedByVisibility) return
  pausedByVisibility = true
  if (app?.ticker) app.ticker.stop()
  stopIdleLoop()
  if (mouthSyncRAF) { cancelAnimationFrame(mouthSyncRAF); mouthSyncRAF = null }
  if (audioSyncRAF) { cancelAnimationFrame(audioSyncRAF); audioSyncRAF = null }
  console.log('[Live2D] Rendering PAUSED (tab hidden)')
}

const resumeRendering = () => {
  if (!pausedByVisibility) return
  pausedByVisibility = false
  if (app?.ticker) app.ticker.start()
  if (isModelLoaded.value) {
    startMouthSyncLoop()
    startIdleLoop()
  }
  console.log('[Live2D] Rendering RESUMED (tab visible)')
}

const handleVisibilityChange = () => {
  pageVisible = !document.hidden
  if (document.hidden) pauseRendering()
  else resumeRendering()
}

// ============ 生命周期 ============
onMounted(async () => {
  if (!initPixiApp()) return
  // CPU 模式降帧到 30fps，减少无 GPU 虚拟机上的渲染压力
  if (app?.ticker) { app.ticker.maxFPS = 30 }
  document.addEventListener('visibilitychange', handleVisibilityChange)
  await loadModel()
})

onUnmounted(() => {
  document.removeEventListener('visibilitychange', handleVisibilityChange)
  stopIdleLoop()
  if (mouthSyncRAF) { cancelAnimationFrame(mouthSyncRAF); mouthSyncRAF = null }
  if (audioSyncRAF) { cancelAnimationFrame(audioSyncRAF); audioSyncRAF = null }
  if (resizeObserver) { resizeObserver.disconnect(); resizeObserver = null }
  if (model) { model.destroy(); model = null }
  if (app) { app.destroy(true, { children: true, texture: true, baseTexture: true }); app = null }
})

defineExpose({ setExpression, playMotion, setSyncMouthOpenY, setEyeTrackingTarget, setEyeOpen, syncMouthFromAudio, retryLoad })
</script>

<style scoped>
.live2d-container { position: relative; width: 100%; height: 100%; background: transparent; overflow: hidden; display: flex; align-items: center; justify-content: center; }
.live2d-container :deep(canvas) { max-width: 100%; max-height: 100%; display: block; margin: auto; }
.loading-overlay, .error-overlay { position: absolute; top: 0; left: 0; width: 100%; height: 100%; display: flex; align-items: center; justify-content: center; background: rgba(0,0,0,0.5); backdrop-filter: blur(5px); z-index: 10; }
.loading-content, .error-content { display: flex; flex-direction: column; align-items: center; gap: 15px; color: white; }
.spinner { width: 40px; height: 40px; border: 4px solid rgba(255,255,255,0.3); border-top-color: white; border-radius: 50%; animation: spin 1s linear infinite; }
@keyframes spin { to { transform: rotate(360deg); } }
.loading-content p, .error-content p { font-size: 16px; font-weight: 500; margin: 0; }
.error-content small { font-size: 12px; opacity: 0.8; text-align: center; max-width: 300px; }
.retry-btn { padding: 8px 20px; border: 1px solid rgba(255,255,255,0.5); border-radius: 6px; background: rgba(255,255,255,0.1); color: white; cursor: pointer; font-size: 14px; transition: background 0.2s; }
.retry-btn:hover { background: rgba(255,255,255,0.25); }
</style>
