import { ref } from 'vue'

interface AudioAnalysisData {
  mouthOpenY: number
  energy: number
  frequency: number
}

export function useAudioPlayer() {
  let audioElement = new Audio()
  const isPlaying = ref(false)
  const currentTime = ref(0)
  const duration = ref(0)
  const volume = ref(1.0)

  // 音频分析
  let audioContext: AudioContext | null = null
  let analyser: AnalyserNode | null = null
  let sourceNode: MediaElementAudioSourceNode | null = null
  let sourceConnected = false  // 追踪 MediaElementSource 是否已连接
  let animationFrameId: number | null = null

  // 事件回调
  let onMouthSync: ((data: AudioAnalysisData) => void) | null = null
  let onAudioEnd: (() => void) | null = null

  /**
   * 初始化音频上下文
   */
  const initAudioContext = () => {
    if (audioContext) return

    try {
      const AudioContextClass = (window as any).AudioContext || (window as any).webkitAudioContext
      audioContext = new AudioContextClass()
      analyser = audioContext.createAnalyser()
      analyser.fftSize = 256

      // 将音频元素连接到分析器
      // createMediaElementSource 对同一个 Audio 元素只能调用一次，
      // 因此用 sourceConnected 标记防止重复调用导致 InvalidStateError
      if (audioContext && analyser && !sourceConnected) {
        sourceNode = audioContext.createMediaElementSource(audioElement)
        sourceNode.connect(analyser)
        analyser.connect(audioContext.destination)
        sourceConnected = true
      }

      console.log('[AudioPlayer] Audio context initialized')
    } catch (error) {
      console.error('[AudioPlayer] Error initializing audio context:', error)
    }
  }

  /**
   * 播放音频并启动分析
   */
  const play = async (audioUrl: string): Promise<void> => {
    return new Promise((resolve, reject) => {
      try {
        audioElement.src = audioUrl
        audioElement.volume = volume.value

        // 设置事件监听
        audioElement.onplay = () => {
          console.log('[AudioPlayer] Playing:', audioUrl)
          isPlaying.value = true
          duration.value = audioElement.duration || 0

          // 启动音频分析
          startAudioAnalysis()
          resolve()
        }

        audioElement.onended = () => {
          console.log('[AudioPlayer] Playback ended')
          isPlaying.value = false
          stop()

          if (onAudioEnd) {
            onAudioEnd()
          }
        }

        audioElement.onerror = (error) => {
          console.error('[AudioPlayer] Error:', error)
          isPlaying.value = false
          reject(new Error('Audio playback error'))
        }

        audioElement.ontimeupdate = () => {
          currentTime.value = audioElement.currentTime
        }

        audioElement.play().catch((error) => {
          console.error('[AudioPlayer] Play error:', error)
          reject(error)
        })
      } catch (error) {
        console.error('[AudioPlayer] Setup error:', error)
        reject(error)
      }
    })
  }

  /**
   * 启动音频分析 (用于嘴部同步)
   */
  const startAudioAnalysis = () => {
    if (!analyser) {
      initAudioContext()
    }

    if (!analyser) return

    const dataArray = new Uint8Array(analyser.frequencyBinCount)

    const updateAnalysis = () => {
      if (!isPlaying.value) return

      analyser!.getByteFrequencyData(dataArray)

      // 计算平均能量
      let sum = 0
      for (let i = 0; i < dataArray.length; i++) {
        sum += dataArray[i]
      }
      const average = sum / dataArray.length

      // 转换为嘴部开合度 (0-1)
      const normalized = Math.min(1, average / 180)
      const mouthOpenY = Math.pow(normalized, 0.6)

      // 计算能量和频率信息
      let lowFreqSum = 0
      let highFreqSum = 0

      for (let i = 0; i < dataArray.length / 2; i++) {
        lowFreqSum += dataArray[i]
      }
      for (let i = dataArray.length / 2; i < dataArray.length; i++) {
        highFreqSum += dataArray[i]
      }

      const energy = lowFreqSum / (dataArray.length / 2) / 255
      const frequency = highFreqSum / (dataArray.length / 2) / 255

      // 调用回调
      if (onMouthSync) {
        onMouthSync({
          mouthOpenY: Math.min(1, mouthOpenY),
          energy,
          frequency
        })
      }

      animationFrameId = requestAnimationFrame(updateAnalysis)
    }

    updateAnalysis()
  }

  /**
   * 暂停播放
   */
  const pause = () => {
    if (audioElement) {
      audioElement.pause()
      isPlaying.value = false
      console.log('[AudioPlayer] Paused')
    }
  }

  /**
   * 停止播放
   */
  const stop = () => {
    if (audioElement) {
      audioElement.pause()
      audioElement.currentTime = 0
    }

    if (animationFrameId !== null) {
      cancelAnimationFrame(animationFrameId)
      animationFrameId = null
    }

    isPlaying.value = false
    currentTime.value = 0
    console.log('[AudioPlayer] Stopped')
  }

  /**
   * 设置音量 (0-1)
   */
  const setVolume = (vol: number) => {
    volume.value = Math.max(0, Math.min(1, vol))
    audioElement.volume = volume.value
  }

  /**
   * 快进/快退
   */
  const seek = (time: number) => {
    audioElement.currentTime = Math.max(0, time)
  }

  /**
   * 设置嘴部同步回调
   */
  const onMouth = (callback: (data: AudioAnalysisData) => void) => {
    onMouthSync = callback
  }

  /**
   * 设置播放结束回调
   */
  const onEnd = (callback: () => void) => {
    onAudioEnd = callback
  }

  // 清理
  const cleanup = () => {
    stop()
    if (audioContext) {
      audioContext.close()
      audioContext = null
      analyser = null
      sourceNode = null
      sourceConnected = false
      // 重建 Audio 元素，因为旧元素已被 createMediaElementSource 绑定过，
      // 无法再次绑定到新的 AudioContext
      audioElement = new Audio()
      audioElement.volume = volume.value
    }
  }

  return {
    // 状态
    isPlaying,
    currentTime,
    duration,
    volume,

    // 方法
    play,
    pause,
    stop,
    setVolume,
    seek,

    // 事件监听
    onMouth,
    onEnd,

    // 清理
    cleanup
  }
}
