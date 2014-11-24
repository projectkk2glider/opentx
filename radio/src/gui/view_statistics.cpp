/*
 * Authors (alphabetical order)
 * - Andre Bernet <bernet.andre@gmail.com>
 * - Andreas Weitl
 * - Bertrand Songis <bsongis@gmail.com>
 * - Bryan J. Rentoul (Gruvin) <gruvin@gmail.com>
 * - Cameron Weeks <th9xer@gmail.com>
 * - Erez Raviv
 * - Gabriel Birkus
 * - Jean-Pierre Parisy
 * - Karl Szmutny
 * - Michael Blandford
 * - Michal Hlavinka
 * - Pat Mackenzie
 * - Philip Moss
 * - Rob Thomson
 * - Romolo Manfredini <romolo.manfredini@gmail.com>
 * - Thomas Husterer
 *
 * opentx is based on code named
 * gruvin9x by Bryan J. Rentoul: http://code.google.com/p/gruvin9x/,
 * er9x by Erez Raviv: http://code.google.com/p/er9x/,
 * and the original (and ongoing) project by
 * Thomas Husterer, th9x: http://code.google.com/p/th9x/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "../opentx.h"

void menuStatisticsView(uint8_t event)
{
  TITLE(STR_MENUSTAT);

  switch(event)
  {
    case EVT_KEY_FIRST(KEY_UP):
      chainMenu(menuStatisticsDebug);
      return;

#if defined(CPUARM)
    case EVT_KEY_LONG(KEY_MENU):
      g_eeGeneral.globalTimer = 0;
      eeDirty(EE_GENERAL);
      sessionTimer = 0;
      break;
#endif
    case EVT_KEY_FIRST(KEY_EXIT):
      chainMenu(menuMainView);
      return;
  }

  lcd_puts(  1*FW, FH*0, STR_TOTTM1TM2THRTHP);
  putsTimer(    5*FW+5*FWNUM+1, FH*1, timersStates[0].val, 0, 0);
  putsTimer(   12*FW+5*FWNUM+1, FH*1, timersStates[1].val, 0, 0);

  putsTimer(    5*FW+5*FWNUM+1, FH*2, s_timeCumThr, 0, 0);
  putsTimer(   12*FW+5*FWNUM+1, FH*2, s_timeCum16ThrP/16, 0, 0);

  putsTimer(   12*FW+5*FWNUM+1, FH*0, sessionTimer, 0, 0);
  
#if defined(CPUARM)
  putsTimer(21*FW+5*FWNUM+1, 0*FH, g_eeGeneral.globalTimer + sessionTimer, TIMEHOUR, 0);
#endif

#if defined(THRTRACE)
  uint8_t traceRd = (s_traceCnt < 0 ? s_traceWr : 0);
  const uint8_t x=5;
  const uint8_t y=60;
  lcd_hline(x-3,y,MAXTRACE+3+3);
  lcd_vline(x,y-32,32+3);

  for (uint8_t i=0; i<MAXTRACE; i+=6) {
    lcd_vline(x+i+6,y-1,3);
  }
  for (uint8_t i=1; i<=MAXTRACE; i++) {
    lcd_vline(x+i, y-s_traceBuf[traceRd], s_traceBuf[traceRd]);
    traceRd++;
    if (traceRd>=MAXTRACE) traceRd = 0;
    if (traceRd==s_traceWr) break;
  }
#endif
}

#if defined(PCBTARANIS)
  #define MENU_DEBUG_COL1_OFS   (11*FW-2)
  #define MENU_DEBUG_Y_MIXMAX   (2*FH-3)
  #define MENU_DEBUG_Y_LUA      (3*FH-2)
  #define MENU_DEBUG_Y_FREE_RAM (4*FH-1)
  #define MENU_DEBUG_Y_STACK    (5*FH)
  #define MENU_DEBUG_Y_RTOS     (6*FH)
#elif defined(PCBSKY9X)
  #define MENU_DEBUG_COL1_OFS   (11*FW-3)
  #define MENU_DEBUG_COL2_OFS   (17*FW)
  #define MENU_DEBUG_Y_CURRENT  (1*FH)
  #define MENU_DEBUG_Y_MAH      (2*FH)
  #define MENU_DEBUG_Y_CPU_TEMP (3*FH)
  #define MENU_DEBUG_Y_COPROC   (4*FH)
  #define MENU_DEBUG_Y_MIXMAX   (5*FH)
  #define MENU_DEBUG_Y_RTOS     (6*FH)
#else
  #define MENU_DEBUG_COL1_OFS   (14*FW)
#endif

void menuStatisticsDebug(uint8_t event)
{
  TITLE(STR_MENUDEBUG);

  switch(event)
  {
#if defined(CPUARM)
    case EVT_KEY_LONG(KEY_ENTER):
      g_eeGeneral.mAhUsed = 0;
      g_eeGeneral.globalTimer = 0;
      eeDirty(EE_GENERAL);
#if defined(PCBSKY9X)
      Current_used = 0;
#endif
      sessionTimer = 0;
      killEvents(event);
      AUDIO_KEYPAD_UP();
      break;
#endif
    case EVT_KEY_FIRST(KEY_ENTER):
#if !defined(CPUARM)
      g_tmr1Latency_min = 0xff;
      g_tmr1Latency_max = 0;
#endif
#if defined(LUA)
      maxLuaInterval = 0;
      maxLuaDuration = 0;
#endif
      maxMixerDuration  = 0;
      AUDIO_KEYPAD_UP();
      break;

#if defined(DEBUG_TIMERS)
    case EVT_KEY_FIRST(KEY_UP):
      chainMenu(menuDebugTimers);
      return;
#endif

#if !defined(DEBUG_TIMERS) && defined(DEBUG_TRACE_BUFFER)
    case EVT_KEY_FIRST(KEY_UP):
      pushMenu(menuTraceBuffer);
      return;
#endif

    case EVT_KEY_FIRST(KEY_DOWN):
      chainMenu(menuStatisticsView);
      return;
    case EVT_KEY_FIRST(KEY_EXIT):
      chainMenu(menuMainView);
      return;
  }

#if defined(PCBSKY9X)
  if ((ResetReason&RSTC_SR_RSTTYP) == (2<<8)) {
    lcd_puts(LCD_W-8*FW, 0*FH, "WATCHDOG");
  }
  else if (unexpectedShutdown) {
    lcd_puts(LCD_W-13*FW, 0*FH, "UNEXP.SHTDOWN");
  }
#endif

#if defined(PCBSKY9X) && !defined(REVA)
  // current
  lcd_putsLeft(MENU_DEBUG_Y_CURRENT, STR_CPU_CURRENT);
  putsTelemetryValue(MENU_DEBUG_COL1_OFS, MENU_DEBUG_Y_CURRENT, getCurrent(), UNIT_MILLIAMPS, LEFT);
  uint32_t current_scale = 488 + g_eeGeneral.currentCalib;
  lcd_putc(MENU_DEBUG_COL2_OFS, MENU_DEBUG_Y_CURRENT, '>');
  putsTelemetryValue(MENU_DEBUG_COL2_OFS+FW+1, MENU_DEBUG_Y_CURRENT, Current_max*10*current_scale/8192, UNIT_RAW, LEFT);
  // consumption
  lcd_putsLeft(MENU_DEBUG_Y_MAH, STR_CPU_MAH);
  putsTelemetryValue(MENU_DEBUG_COL1_OFS, MENU_DEBUG_Y_MAH, g_eeGeneral.mAhUsed + Current_used*current_scale/8192/36, UNIT_MAH, LEFT|PREC1);
#endif

#if defined(PCBSKY9X)
  lcd_putsLeft(MENU_DEBUG_Y_CPU_TEMP, STR_CPU_TEMP);
  putsTelemetryValue(MENU_DEBUG_COL1_OFS, MENU_DEBUG_Y_CPU_TEMP, getTemperature(), UNIT_TEMPERATURE, LEFT);
  lcd_putc(MENU_DEBUG_COL2_OFS, MENU_DEBUG_Y_CPU_TEMP, '>');
  putsTelemetryValue(MENU_DEBUG_COL2_OFS+FW+1, MENU_DEBUG_Y_CPU_TEMP, maxTemperature+g_eeGeneral.temperatureCalib, UNIT_TEMPERATURE, LEFT);
#endif

#if defined(COPROCESSOR)
  lcd_putsLeft(MENU_DEBUG_Y_COPROC, STR_COPROC_TEMP);

  if (Coproc_read==0) {
    lcd_putsAtt(MENU_DEBUG_COL1_OFS, MENU_DEBUG_Y_COPROC, PSTR("Co Proc NACK"),INVERS);
  }
  else if (Coproc_read==0x81) {
    lcd_putsAtt(MENU_DEBUG_COL1_OFS, MENU_DEBUG_Y_COPROC, PSTR("Inst.TinyApp"),INVERS);
  }
  else if (Coproc_read<3) {
    lcd_putsAtt(MENU_DEBUG_COL1_OFS, MENU_DEBUG_Y_COPROC, PSTR("Upgr.TinyApp"),INVERS);
  }
  else {
    putsTelemetryValue(MENU_DEBUG_COL1_OFS, MENU_DEBUG_Y_COPROC, Coproc_temp, UNIT_TEMPERATURE, LEFT);
    putsTelemetryValue(MENU_DEBUG_COL2_OFS, MENU_DEBUG_Y_COPROC, Coproc_maxtemp, UNIT_TEMPERATURE, LEFT);
  }
#endif

#if defined(PCBTARANIS) && !defined(SIMU)
  lcd_putsLeft(MENU_DEBUG_Y_FREE_RAM, "Free Mem");
  lcd_outdezAtt(MENU_DEBUG_COL1_OFS, MENU_DEBUG_Y_FREE_RAM, getAvailableMemory(), LEFT);
  lcd_puts(lcdLastPos, MENU_DEBUG_Y_FREE_RAM, "b");
#endif

#if defined(LUA)
  lcd_putsLeft(MENU_DEBUG_Y_LUA, "Lua scripts");
  lcd_putsAtt(MENU_DEBUG_COL1_OFS, MENU_DEBUG_Y_LUA+1, "[Duration]", SMLSIZE);
  lcd_outdezAtt(lcdLastPos, MENU_DEBUG_Y_LUA, 10*maxLuaDuration, LEFT);
  lcd_putsAtt(lcdLastPos+2, MENU_DEBUG_Y_LUA+1, "[Interval]", SMLSIZE);
  lcd_outdezAtt(lcdLastPos, MENU_DEBUG_Y_LUA, 10*maxLuaInterval, LEFT);
#endif

#if defined(CPUARM)
  lcd_putsLeft(MENU_DEBUG_Y_MIXMAX, STR_TMIXMAXMS);
  lcd_outdezAtt(MENU_DEBUG_COL1_OFS, MENU_DEBUG_Y_MIXMAX, DURATION_MS_PREC2(maxMixerDuration), PREC2|LEFT);
  lcd_puts(lcdLastPos, MENU_DEBUG_Y_MIXMAX, "ms");
#endif

#if 0
  lcd_putsLeft(MENU_DEBUG_Y_STACK, STR_FREESTACKMINB);
  lcd_outdezAtt(MENU_DEBUG_COL1_OFS, MENU_DEBUG_Y_STACK, stack_free(255), UNSIGN|LEFT);
  lcd_puts(lcdLastPos, MENU_DEBUG_Y_STACK, "b");
#endif

#if defined(PCBTARANIS)
  lcd_putsLeft(MENU_DEBUG_Y_RTOS, STR_FREESTACKMINB);
  lcd_putsAtt(MENU_DEBUG_COL1_OFS, MENU_DEBUG_Y_RTOS+1, "[Main]", SMLSIZE);
  lcd_outdezAtt(lcdLastPos, MENU_DEBUG_Y_RTOS, stack_free(0), UNSIGN|LEFT);
  lcd_putsAtt(lcdLastPos+2, MENU_DEBUG_Y_RTOS+1, "[Mix]", SMLSIZE);
  lcd_outdezAtt(lcdLastPos, MENU_DEBUG_Y_RTOS, stack_free(1), UNSIGN|LEFT);
  lcd_putsAtt(lcdLastPos+2, MENU_DEBUG_Y_RTOS+1, "[Audio]", SMLSIZE);
  lcd_outdezAtt(lcdLastPos, MENU_DEBUG_Y_RTOS, stack_free(2), UNSIGN|LEFT);
#endif

#if defined(PCBSKY9X)
  lcd_putsLeft(MENU_DEBUG_Y_RTOS, STR_FREESTACKMINB);
  lcd_outdezAtt(MENU_DEBUG_COL1_OFS, MENU_DEBUG_Y_RTOS, stack_free(0), UNSIGN|LEFT);
  lcd_puts(lcdLastPos, MENU_DEBUG_Y_RTOS, "/");
  lcd_outdezAtt(lcdLastPos, MENU_DEBUG_Y_RTOS, stack_free(1), UNSIGN|LEFT);
  lcd_puts(lcdLastPos, MENU_DEBUG_Y_RTOS, "/");
  lcd_outdezAtt(lcdLastPos, MENU_DEBUG_Y_RTOS, stack_free(2), UNSIGN|LEFT);
#endif

#if !defined(CPUARM)
  lcd_putsLeft(1*FH, STR_TMR1LATMAXUS);
  lcd_outdez8(MENU_DEBUG_COL1_OFS , 1*FH, g_tmr1Latency_max/2 );
  lcd_putsLeft(2*FH, STR_TMR1LATMINUS);
  lcd_outdez8(MENU_DEBUG_COL1_OFS , 2*FH, g_tmr1Latency_min/2 );
  lcd_putsLeft(3*FH, STR_TMR1JITTERUS);
  lcd_outdez8(MENU_DEBUG_COL1_OFS , 3*FH, (g_tmr1Latency_max - g_tmr1Latency_min) /2 );
  lcd_putsLeft(4*FH, STR_TMIXMAXMS);
  lcd_outdezAtt(MENU_DEBUG_COL1_OFS, 4*FH, DURATION_MS_PREC2(maxMixerDuration), PREC2);
  lcd_putsLeft(5*FH, STR_FREESTACKMINB);
  lcd_outdezAtt(14*FW, 5*FH, stack_free(), UNSIGN) ;
#endif

  lcd_puts(3*FW, 7*FH+1, STR_MENUTORESET);
  lcd_status_line();
}


#if defined(DEBUG_TRACE_BUFFER)
#include "stamp-opentx.h"

void menuTraceBuffer(uint8_t event)
{
  switch(event)
  {
    case EVT_KEY_LONG(KEY_ENTER):
      dumpTraceBuffer();
      killEvents(event);
      break;
  }

  SIMPLE_SUBMENU("Trace Buffer " VERS_STR, TRACE_BUFFER_LEN);
  /* RTC time */
  struct gtm t;
  gettime(&t);
  putsTime(LCD_W+1, 0, t, TIMEBLINK);

  uint8_t y = 0;
  uint8_t k = 0;
  int8_t sub = m_posVert;

  lcd_putc(0, FH, '#');
  lcd_puts(4*FW, FH, "Time");
  lcd_puts(14*FW, FH, "Event");
  lcd_puts(20*FW, FH, "Data");

  for (uint8_t i=0; i<LCD_LINES-2; i++) {
    y = 1 + (i+2)*FH;
    k = i+s_pgOfs;

    //item
    lcd_outdezAtt(0, y, k, LEFT | (sub==k ? INVERS : 0));

    const struct TraceElement * te = getTraceElement(k);
    if (te) {
      //time
      putstime_t tme = te->time % SECS_PER_DAY;
      putsTimer(4*FW, y, tme, TIMEHOUR|LEFT, TIMEHOUR|LEFT);
      //event
      lcd_outdezNAtt(14*FW, y, te->event, LEADING0|LEFT, 3);
      //data
      lcd_putsn  (20*FW, y, "0x", 2);
      lcd_outhex4(22*FW-2, y, (uint16_t)(te->data >> 16));
      lcd_outhex4(25*FW, y, (uint16_t)(te->data & 0xFFFF));
    }

  }


}
#endif //#if defined(DEBUG_TRACE_BUFFER)


#if defined(DEBUG_TIMERS)

#include "debug_timers.h"

extern DebugTimer debugTimer1;
extern DebugTimer debugTimer2;
extern DebugTimer debugTimer3;
extern DebugTimer debugTimer4;

void displayDuration(int x, int y, debug_timer_t duration) 
{
  if (duration >= 100000) {
    lcd_putsAtt(x, y, "^", 0);
  }
  else if (duration >= 300) {
    lcd_outdezAtt(x, y, duration/10, LEFT);  
  }
  else {
    lcd_outdezAtt(x, y, duration, LEFT|PREC1);  
  }
}

void displayTimer(int y, const char * title, const DebugTimer * Duration, const DebugTimer * Period) 
{
  lcd_putsAtt(0, y+1, title, SMLSIZE);
  if (Duration) {
    lcd_putsAtt(MENU_DEBUG_COL1_OFS, y+1, "Dur", SMLSIZE);
    displayDuration(lcdLastPos, y, Duration->getMin());
    // displayDuration(lcdLastPos, y, Duration->getLast());
    lcd_putsAtt(lcdLastPos, y+1, "_", SMLSIZE);
    displayDuration(lcdLastPos, y, Duration->getMax());
  }
  if (Period) {
    lcd_putsAtt(lcdLastPos+2, y+1, "Int", SMLSIZE);
    displayDuration(lcdLastPos, y, Period->getMin());
    lcd_putsAtt(lcdLastPos, y+1, "_", SMLSIZE);
    displayDuration(lcdLastPos, y, Period->getMax());
  }
}

void menuDebugTimers(uint8_t event)
{
  // TITLE(STR_MENUDEBUG);
  TITLE("DEBUG TIMERS");

  switch(event)
  {
    case EVT_KEY_FIRST(KEY_ENTER):
      //reset timers
      debugTimer1.reset();
      debugTimer2.reset();
      debugTimer3.reset();
      debugTimer4.reset();
      AUDIO_KEYPAD_UP();
      break;

#if defined(DEBUG_TRACE_BUFFER)
    case EVT_KEY_FIRST(KEY_UP):
      pushMenu(menuTraceBuffer);
      return;
#endif

    case EVT_KEY_FIRST(KEY_DOWN):
      chainMenu(menuStatisticsView);
      return;
    case EVT_KEY_FIRST(KEY_EXIT):
      chainMenu(menuMainView);
      return;
  }

  int y = 2*FH-5; 
  displayTimer(y, "1 and 2", &debugTimer1, &debugTimer2); y += FH;
  displayTimer(y, "2 and 4", &debugTimer3, &debugTimer4); y += FH;

  lcd_puts(3*FW, 7*FH+1, STR_MENUTORESET);
  lcd_status_line();
}

#endif  //#if defined(DEBUG_TIMERS)

