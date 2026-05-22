import { ref } from 'vue'

/**
 * 字幕渐进显示逻辑
 *
 * 根据音频时长逐字刻出字幕，支持 Unicode / emoji 字符。
 * 从 LiveStream.vue 中提取，便于复用和测试。
 */
export function useSubtitle() {
  const subtitleText = ref('')
  let subtitleTimer: ReturnType<typeof setTimeout> | null = null

  /**
   * 启动渐进式字幕
   * @param fullText 完整字幕文本
   * @param durationMs 音频时长（毫秒）
   */
  const startProgressive = (fullText: string, durationMs: number) => {
    clear()

    const chars = Array.from(fullText)
    const totalChars = chars.length
    if (totalChars === 0) return

    // 每个字符的间隔时间，留出 10% 尾部时间保持全文显示
    const charInterval = Math.max(50, (durationMs * 0.9) / totalChars)
    let currentIndex = 0

    subtitleText.value = chars[0]
    currentIndex = 1

    const tick = () => {
      if (currentIndex < totalChars) {
        subtitleText.value = chars.slice(0, currentIndex + 1).join('')
        currentIndex++
        subtitleTimer = setTimeout(tick, charInterval)
      }
    }

    subtitleTimer = setTimeout(tick, charInterval)
  }

  /**
   * 立即显示完整文本（无动画）
   */
  const setImmediate = (text: string) => {
    clear()
    subtitleText.value = text
  }

  /**
   * 清除字幕 + 取消定时器
   */
  const clear = () => {
    clearTimer()
    subtitleText.value = ''
  }

  const clearTimer = () => {
    if (subtitleTimer !== null) {
      clearTimeout(subtitleTimer)
      subtitleTimer = null
    }
  }

  return {
    subtitleText,
    startProgressive,
    setImmediate,
    clear,
    clearTimer
  }
}
