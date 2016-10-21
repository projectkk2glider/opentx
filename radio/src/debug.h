/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <inttypes.h>
#include "rtc.h"
#include "dump.h"
#if defined(CLI)
#include "cli.h"
#elif defined(CPUARM)
#include "serial.h"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(SIMU)
typedef void (*traceCallbackFunc)(const char * text);
extern traceCallbackFunc traceCallback;
void debugPrintf(const char * format, ...);
#elif defined(DEBUG) && defined(CLI) && defined(USB_SERIAL)
#define debugPrintf(...) do { if (cliTracesEnabled) serialPrintf(__VA_ARGS__); } while(0)
#elif defined(DEBUG) && defined(CLI)
uint8_t serial2TracesEnabled();
#define debugPrintf(...) do { if (serial2TracesEnabled() && cliTracesEnabled) serialPrintf(__VA_ARGS__); } while(0)
#elif defined(DEBUG) && defined(CPUARM)
uint8_t serial2TracesEnabled();
#define debugPrintf(...) do { if (serial2TracesEnabled()) serialPrintf(__VA_ARGS__); } while(0)
#else
#define debugPrintf(...)
#endif

#if defined(__cplusplus)
}
#endif

#define TRACE_NOCRLF(...)     do { debugPrintf(__VA_ARGS__); } while(0)
#define TRACE(...)            do { debugPrintf(__VA_ARGS__); debugPrintf("\r\n"); } while(0)
#define DUMP(data, size)      dump(data, size)
#define TRACE_DEBUG(...)      debugPrintf("-D- " __VA_ARGS__)
#define TRACE_DEBUG_WP(...)   debugPrintf(__VA_ARGS__)
#define TRACE_INFO(...)       debugPrintf("-I- " __VA_ARGS__)
#define TRACE_INFO_WP(...)    debugPrintf(__VA_ARGS__)
#define TRACE_WARNING(...)    debugPrintf("-W- " __VA_ARGS__)
#define TRACE_WARNING_WP(...) debugPrintf(__VA_ARGS__)
#define TRACE_ERROR(...)      debugPrintf("-E- " __VA_ARGS__)

#if defined(DEBUG) && !defined(SIMU)
#define TIME_MEASURE_START(id) uint16_t t0 ## id = getTmr2MHz()
#define TIME_MEASURE_STOP(id)  TRACE("Measure(" # id ") = %.1fus", float((uint16_t)(getTmr2MHz() - t0 ## id))/2)
#else
#define TIME_MEASURE_START(id)
#define TIME_MEASURE_STOP(id)
#endif

#if defined(DEBUG_TRACE_BUFFER)

#define TRACE_BUFFER_LEN  50

enum TraceEvent {
  trace_start = 1,

  sd_wait_ready = 10,
  sd_rcvr_datablock,
  sd_xmit_datablock_wait_ready,
  sd_xmit_datablock_rcvr_spi,
  sd_send_cmd_wait_ready,
  sd_send_cmd_rcvr_spi,

  sd_SD_ReadSectors = 16,
  sd_disk_read,
  sd_SD_WriteSectors,
  sd_disk_write,

  sd_disk_ioctl_CTRL_SYNC = 20,
  sd_disk_ioctl_GET_SECTOR_COUNT,
  sd_disk_ioctl_MMC_GET_CSD,
  sd_disk_ioctl_MMC_GET_CID,
  sd_disk_ioctl_MMC_GET_OCR,
  sd_disk_ioctl_MMC_GET_SDSTAT_1,
  sd_disk_ioctl_MMC_GET_SDSTAT_2,
  sd_spi_reset,
  sd_wait_read,
  sd_wait_write,
  sd_irq,

  ff_f_write_validate = 40,
  ff_f_write_flag,
  ff_f_write_clst,
  ff_f_write_sync_window,
  ff_f_write_disk_write_dirty,
  ff_f_write_clust2sect,
  ff_f_write_disk_write,
  ff_f_write_disk_read,
  ff_f_write_move_window,

  audio_getNextFilledBuffer_skip = 60,
};

struct TraceElement {
  gtime_t time;
  uint8_t time_ms;
  enum TraceEvent event;
  uint32_t data;
};

#if defined(__cplusplus)
extern "C" {
#endif
void trace_event(enum TraceEvent event, uint32_t data);
void trace_event_i(enum TraceEvent event, uint32_t data);
const struct TraceElement * getTraceElement(uint16_t idx);
void dumpTraceBuffer();
#if defined(__cplusplus)
}
#endif

#define TRACE_EVENT(condition, event, data)   if (condition) { trace_event(event, data); }
#define TRACEI_EVENT(condition, event, data)  if (condition) { trace_event_i(event, data); }

#else  // #if defined(DEBUG_TRACE_BUFFER)

#define TRACE_EVENT(condition, event, data)  
#define TRACEI_EVENT(condition, event, data)  

#endif // #if defined(DEBUG_TRACE_BUFFER)

#if defined(TRACE_SD_CARD)
  #define TRACE_SD_CARD_EVENT(condition, event, data)  TRACE_EVENT(condition, event, data)
#else
  #define TRACE_SD_CARD_EVENT(condition, event, data)  
#endif
#if defined(TRACE_FATFS)
  #define TRACE_FATFS_EVENT(condition, event, data)  TRACE_EVENT(condition, event, data)
#else
  #define TRACE_FATFS_EVENT(condition, event, data)  
#endif
#if defined(TRACE_AUDIO)
  #define TRACE_AUDIO_EVENT(condition, event, data)  TRACE_EVENT(condition, event, data)
  #define TRACEI_AUDIO_EVENT(condition, event, data) TRACEI_EVENT(condition, event, data)
#else
  #define TRACE_AUDIO_EVENT(condition, event, data)  
  #define TRACEI_AUDIO_EVENT(condition, event, data)  
#endif


#if defined(JITTER_MEASURE)  && defined(__cplusplus)

template<class T> class JitterMeter {
public:
  T min;
  T max;
  T measured;

  JitterMeter() : min(~(T)0), max(0), measured(0) {};

  void reset() {
    // store mesaurement
    measured = max - min;
    //reset - begin new measurement
    min = ~(T)0;
    max = 0;
  };

  void measure(T value) {
    if (value > max) max = value;
    if (value < min) min = value;
  };

  T get() const {
    return measured;
  };
};

#endif  // defined(JITTER_MEASURE)


#if defined(DEBUG_INTERRUPTS) && !defined(BOOT)

#if defined(PCBHORUS)
enum InterruptNames {
  INT_TICK,
  INT_1MS,
  INT_SER2,
  INT_TELEM_DMA,
  INT_TELEM_USART,
  INT_SDIO,
  INT_SDIO_DMA,
  INT_DMA2S7,
  INT_TIM1CC,
  INT_TIM2,
  INT_TRAINER,
  INT_OTG_FS,
  INT_LAST
};
#elif defined(PCBTARANIS)
enum InterruptNames {
  INT_TICK,
  INT_5MS,
  INT_AUDIO,
  INT_BLUETOOTH,
  INT_LCD,
  INT_TIM1CC,
  INT_TIM1,
  INT_TIM8,
  INT_SER2,
  INT_TELEM_DMA,
  INT_TELEM_USART,
  INT_TRAINER,
  INT_OTG_FS,
  INT_LAST
};
#endif

struct InterruptCounters
{
  uint32_t cnt[INT_LAST];
  uint32_t resetTime;
};

extern const char * interruptNames[INT_LAST];
extern struct InterruptCounters interruptCounters;

#define DEBUG_INTERRUPT(int)    (++interruptCounters.cnt[int])
#else
#define DEBUG_INTERRUPT(int)
#endif //#if defined(DEBUG_INTERRUPTS)

#if defined(DEBUG_TASKS)

#define DEBUG_TASKS_LOG_SIZE    512

// each 32bit is used as:
//    top 8 bits: task id
//    botom 24 bits: system tick counter
extern uint32_t taskSwitchLog[DEBUG_TASKS_LOG_SIZE];
extern uint16_t taskSwitchLogPos;

#if defined(__cplusplus)
extern "C" {
#endif
extern void CoTaskSwitchHook(uint8_t taskID);
#if defined(__cplusplus)
}
#endif

#endif // #if defined(DEBUG_TASKS)


#if defined(DEBUG_TIMERS)


  #if defined(__cplusplus)
  extern "C" {
  #endif

enum DebugTimers {
  DT_INT_TICK,
  DT_1MS,
  DT_USB,
  DT_DMA_INTMODULE,
  DT_TIM_INTMODULE,
  DT_SDIO,
  DT_SDIO_DMA,
  DT_TELEM_DMA,
  DT_TELEM_USART,
  DT_SER2,
  DT_TRAINER,
  DT_EXT_DMA,
  DT_EXT_TIMER,
  DT_GPS,

  DEBUG_TIMERS_COUNT
};

    void dt_start(uint8_t timer);
    void dt_stop(uint8_t timer);
    void dt_sample(uint8_t timer);

    #define C_DEBUG_TIMER_START(timer)  dt_start(timer);
    #define C_DEBUG_TIMER_STOP(timer)   dt_stop(timer);
    #define C_DEBUG_TIMER_SAMPLE(timer) dt_sample(timer)
    #define DEBUG_TIMER_START(timer)  
    #define DEBUG_TIMER_STOP(timer)  
    #define DEBUG_TIMER_SAMPLE(timer) 

  #if defined(__cplusplus)
  }
  #endif


#if defined(__cplusplus)
typedef uint32_t debug_timer_t;

class DebugTimer
{
private:
  debug_timer_t min;
  debug_timer_t max;
  // debug_timer_t avg;
  debug_timer_t last;   //unit 1us

  uint16_t _start_hiprec;
  uint32_t _start_loprec;

  void evalStats() {
    if (min > last) min = last;
    if (max < last) max = last;
    //todo avg
  }

public:
  DebugTimer(): min(-1), max(0), /*avg(0),*/ last(0), _start_hiprec(0), _start_loprec(0) {};

  void start();
  void stop();
  void sample() { stop(); start(); }

  void reset() { min = -1;  max = last = 0; }

  debug_timer_t getMin() const { return min; }
  debug_timer_t getMax() const { return max; }
  debug_timer_t getLast() const { return last; }
};


extern DebugTimer debugTimers[DEBUG_TIMERS_COUNT];
extern const char * debugTimerNames[DEBUG_TIMERS_COUNT];

#endif // #if defined(__cplusplus)

#else //#if defined(DEBUG_TIMERS)

#define DEBUG_TIMER_START(timer)
#define DEBUG_TIMER_STOP(timer)
#define DEBUG_TIMER_SAMPLE(timer)

#endif //#if defined(DEBUG_TIMERS)

#endif // _DEBUG_H_

