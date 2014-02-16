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

#include "opentx.h"

PACK(typedef struct {
  uint8_t  mode;         // 0=end, 1=pos, 2=neg, 3=both
  uint8_t  chn;
  int8_t   swtch;
  uint16_t phases;
  int8_t   weight;
  uint8_t  curveMode;
  char     name[LEN_EXPOMIX_NAME];
  uint8_t  spare[2];
  int8_t   curveParam;
}) ExpoData_v215;

#if defined(PCBTARANIS)
  #define LIMITDATA_V215_EXTRA  char name[LEN_CHANNEL_NAME];
#else
  #define LIMITDATA_V215_EXTRA
#endif

PACK(typedef struct {
  int8_t  min;
  int8_t  max;
  int8_t  ppmCenter;
  int16_t offset:14;
  uint16_t symetrical:1;
  uint16_t revert:1;
  LIMITDATA_V215_EXTRA
}) LimitData_v215;

#if defined(PCBTARANIS)
PACK(typedef struct {
  uint8_t  destCh;
  uint16_t phases;
  uint8_t  curveMode:1;       // O=curve, 1=differential
  uint8_t  noExpo:1;
  int8_t   carryTrim:3;
  uint8_t  mltpx:2;           // multiplex method: 0 means +=, 1 means *=, 2 means :=
  uint8_t  spare1:1;
  int16_t  weight;
  int8_t   swtch;
  int8_t   curveParam;
  uint8_t  mixWarn:4;         // mixer warning
  uint8_t  srcVariant:4;
  uint8_t  delayUp;
  uint8_t  delayDown;
  uint8_t  speedUp;
  uint8_t  speedDown;
  uint8_t  srcRaw;
  int16_t  offset;
  char     name[LEN_EXPOMIX_NAME];
  uint8_t  spare2[2];
}) MixData_v215;
#else
PACK(typedef struct {
  uint8_t  destCh;
  uint16_t phases;
  uint8_t  curveMode:1;       // O=curve, 1=differential
  uint8_t  noExpo:1;
  int8_t   carryTrim:3;
  uint8_t  mltpx:2;           // multiplex method: 0 means +=, 1 means *=, 2 means :=
  uint8_t  spare:1;
  int16_t  weight;
  int8_t   swtch;
  int8_t   curveParam;
  uint8_t  mixWarn:4;         // mixer warning
  uint8_t  srcVariant:4;
  uint8_t  delayUp;
  uint8_t  delayDown;
  uint8_t  speedUp;
  uint8_t  speedDown;
  uint8_t  srcRaw;
  int16_t  offset;
  char     name[LEN_EXPOMIX_NAME];
}) MixData_v215;
#endif

PACK(typedef struct {
  int8_t    mode;            // timer trigger source -> off, abs, stk, stk%, sw/!sw, !m_sw/!m_sw
  uint16_t  start:12;
  uint16_t  countdownBeep:1;
  uint16_t  minuteBeep:1;
  uint16_t  persistent:1;
  uint16_t  spare:1;
  uint16_t  value;
}) TimerData_v215;

PACK(typedef struct {
  int16_t trim[4];
  int8_t swtch;       // swtch of phase[0] is not used
  char name[LEN_FP_NAME];
  uint8_t fadeIn;
  uint8_t fadeOut;
  ROTARY_ENCODER_ARRAY;
  gvar_t gvars[5];
}) PhaseData_v215;

PACK(typedef struct {
  int16_t v1;
  int16_t v2;
  uint8_t func;
  uint8_t delay;
  uint8_t duration;
  int8_t  andsw;
}) LogicalSwitchData_v215;

PACK(typedef struct {
  int8_t  swtch;
  uint8_t func;
  PACK(union {
    char name[LEN_CFN_NAME];
    struct {
      int16_t val;
      int16_t ext1;
      int16_t ext2;
    } composite;
  }) param;
  uint8_t mode:2;
  uint8_t active:6;
}) CustomFnData_v215;

PACK(typedef struct {
  FrSkyChannelData channels[2];
  uint8_t usrProto; // Protocol in FrSky user data, 0=None, 1=FrSky hub, 2=WS HowHigh, 3=Halcyon
  uint8_t voltsSource;
  uint8_t blades;   // How many blades for RPMs, 0=2 blades, 1=3 blades
  uint8_t currentSource;
  uint8_t screensType;
  FrSkyScreenData screens[MAX_FRSKY_SCREENS];
  uint8_t varioSource;
  int8_t  varioCenterMax;
  int8_t  varioCenterMin;
  int8_t  varioMin;
  int8_t  varioMax;
  FrSkyRSSIAlarm rssiAlarms[2];
}) FrSkyData_v215;

PACK(typedef struct {
  ModelHeader header;
  TimerData_v215 timers[MAX_TIMERS];
  uint8_t   protocol:3;
  uint8_t   thrTrim:1;            // Enable Throttle Trim
  int8_t    ppmNCH:4;
  uint8_t   trimInc:3;            // Trim Increments
  uint8_t   disableThrottleWarning:1;
  uint8_t   pulsePol:1;
  uint8_t   extendedLimits:1;
  uint8_t   extendedTrims:1;
  uint8_t   throttleReversed:1;
  int8_t    ppmDelay;
  BeepANACenter beepANACenter;        // 1<<0->A1.. 1<<6->A7
  MixData_v215   mixData[MAX_MIXERS];
  LimitData_v215 limitData[NUM_CHNOUT];
  ExpoData_v215  expoData[32];

  int16_t   curves[16];
  int8_t    points[NUM_POINTS];

  LogicalSwitchData_v215 customSw[NUM_CSW];
  CustomFnData_v215 funcSw[NUM_CFN];
  SwashRingData swashR;
  PhaseData_v215 phaseData[MAX_PHASES];

  int8_t    ppmFrameLength;       // 0=22.5ms  (10ms-30ms) 0.5ms increments
  uint8_t   thrTraceSrc;

  swstate_t switchWarningStates;

  char gvar_names[5][LEN_GVAR_NAME];

  FrSkyData_v215 frsky;

  ROTARY_ENCODER_ARRAY_EXTRA

  MODELDATA_EXTRA

}) ModelData_v215;

void ConvertGeneralSettings_215_to_216(EEGeneral &settings)
{
  settings.version = 216;
}

#if defined(PCBTARANIS)
int ConvertSource_215_to_216(int source, bool insertZero=false)
{
  if (insertZero)
    source += 1;
  // Virtual Inputs and Lua Outputs added
  if (source > 0)
    source += MAX_INPUTS + MAX_SCRIPTS*MAX_SCRIPT_OUTPUTS;
  // 4 GVARS added
  if (source > MIXSRC_GVAR1+4)
    source += 4;
  // ASpd and dTE added
  if (source >= MIXSRC_FIRST_TELEM-1+TELEM_ASPD)
    source += 2;
  // Cel- and Vfas- added
  if (source >= MIXSRC_FIRST_TELEM-1+TELEM_MIN_CELL)
    source += 2;

  return source;
}

int ConvertSwitch_215_to_216(int swtch)
{
  if (swtch < 0)
    return -ConvertSwitch_215_to_216(-swtch);
  else if (swtch <= SWSRC_LAST_SWITCH)
    return swtch;
  else
    return swtch + (2*4) + (2*6); // 4 trims and 2 * 6-pos added as switches
}
#else
int ConvertSource_215_to_216(int source, bool insertZero=false)
{
  if (insertZero)
    source += 1;
  // 4 GVARS added
  if (source > MIXSRC_GVAR1+4)
    source += 4;
  // ASpd and dTE added
  if (source >= MIXSRC_FIRST_TELEM-1+TELEM_ASPD)
    source += 2;
  // Cel- and Vfas- added
  if (source >= MIXSRC_FIRST_TELEM-1+TELEM_MIN_CELL)
    source += 2;
  return source;
}

int ConvertSwitch_215_to_216(int swtch)
{
  if (swtch <= SWSRC_LAST_SWITCH)
    return swtch;
  else
    return swtch + (2*4) + 1; // 4 trims and REa added
}
#endif

void ConvertModel_215_to_216(ModelData &model)
{
  // Virtual inputs added instead of Expo/DR
  // LUA scripts added
  // GVARS: now 9 GVARS, popup param added
  // Curves: structure changed, 32 curves
  // Limits: min and max with PREC1
  // Custom Functions: play repeat * 5
  // Custom Switches: better precision for x when A comes from telemetry
  // Main View: altitude in top bar
  // Mixes: GVARS in weight moved from 512 to 4096 and -512 to -4096, because GVARS may be used in limits [-1250:1250]
  // Switches: two 6-pos pots added, REa added to Sky9x

  assert(sizeof(ModelData_v215) <= sizeof(ModelData));

  ModelData_v215 oldModel;
  memcpy(&oldModel, &model, sizeof(oldModel));
  memset(&model, 0, sizeof(ModelData));

  char name[LEN_MODEL_NAME+1];
  zchar2str(name, oldModel.header.name, LEN_MODEL_NAME);
  TRACE("Model %s conversion from v215 to v216", name);

  memcpy(&g_model.header, &oldModel.header, sizeof(g_model.header));
  for (uint8_t i=0; i<2; i++) {
    TimerData & timer = g_model.timers[i];
    if (oldModel.timers[i].mode >= TMRMODE_FIRST_SWITCH)
      timer.mode = TMRMODE_FIRST_SWITCH + ConvertSwitch_215_to_216(oldModel.timers[i].mode - TMRMODE_FIRST_SWITCH + 1) - 1;
    else
      timer.mode = oldModel.timers[i].mode;
    timer.start = oldModel.timers[i].start;
    timer.minuteBeep = oldModel.timers[i].minuteBeep;
    timer.persistent = oldModel.timers[i].persistent;
    timer.countdownBeep = oldModel.timers[i].countdownBeep;
    timer.value = oldModel.timers[i].value;
  }
  g_model.protocol = oldModel.protocol;
  g_model.thrTrim = oldModel.thrTrim;
  g_model.trimInc = oldModel.trimInc - 2;
  g_model.disableThrottleWarning = oldModel.disableThrottleWarning;
  g_model.extendedLimits = oldModel.extendedLimits;
  g_model.extendedTrims = oldModel.extendedTrims;
  g_model.throttleReversed = oldModel.throttleReversed;
  g_model.beepANACenter = oldModel.beepANACenter;

  for (uint8_t i=0; i<64; i++) {
    MixData & mix = g_model.mixData[i];
#if defined(PCBTARANIS)
    mix.destCh = oldModel.mixData[i].destCh;
    mix.phases = oldModel.mixData[i].phases;
    mix.mltpx = oldModel.mixData[i].mltpx;
    mix.weight = oldModel.mixData[i].weight;
    mix.swtch = ConvertSwitch_215_to_216(oldModel.mixData[i].swtch);
    if (oldModel.mixData[i].curveMode==0/*differential*/) {
      mix.curve.type = CURVE_REF_DIFF;
      mix.curve.value = oldModel.mixData[i].curveParam;
    }
    else if (oldModel.mixData[i].curveParam <= 6) {
      mix.curve.type = CURVE_REF_FUNC;
      mix.curve.value = oldModel.mixData[i].curveParam;
    }
    else {
      mix.curve.type = CURVE_REF_CUSTOM;
      mix.curve.value = oldModel.mixData[i].curveParam - 6;
    }
    mix.mixWarn = oldModel.mixData[i].mixWarn;
    mix.delayUp = oldModel.mixData[i].delayUp;
    mix.delayDown = oldModel.mixData[i].delayDown;
    mix.speedUp = oldModel.mixData[i].speedUp;
    mix.speedDown = oldModel.mixData[i].speedDown;
    mix.srcRaw = oldModel.mixData[i].srcRaw;
    if (mix.srcRaw > 4 || oldModel.mixData[i].noExpo)
      mix.srcRaw = ConvertSource_215_to_216(mix.srcRaw);
    mix.offset = oldModel.mixData[i].offset;
    memcpy(mix.name, oldModel.mixData[i].name, LEN_EXPOMIX_NAME);
#else
    memcpy(&mix, &oldModel.mixData[i], sizeof(mix));
#endif
    if (mix.weight <= -508)
      mix.weight = mix.weight + 512 - 4096;
    else if (mix.weight >= 507)
      mix.weight = mix.weight - 512 + 4096;
    else
      mix.offset = mix.offset * mix.weight / 100;
  }
  for (uint8_t i=0; i<32; i++) {
    g_model.limitData[i].min = 10 * oldModel.limitData[i].min;
    g_model.limitData[i].max = 10 * oldModel.limitData[i].max;
    g_model.limitData[i].ppmCenter = oldModel.limitData[i].ppmCenter;
    g_model.limitData[i].offset = oldModel.limitData[i].offset;
    g_model.limitData[i].symetrical = oldModel.limitData[i].symetrical;
    g_model.limitData[i].revert = oldModel.limitData[i].revert;
#if defined(PCBTARANIS)
    memcpy(&g_model.limitData[i].name, &oldModel.limitData[i].name, LEN_CHANNEL_NAME);
#endif
  }
  int indexes[NUM_STICKS] = { 0, 0, 0, 0 };
  for (uint8_t i=0; i<32; i++) {
    if (oldModel.expoData[i].mode) {
#if defined(PCBTARANIS)
      uint8_t chn = oldModel.expoData[i].chn;
      if (!oldModel.expoData[i].swtch && !oldModel.expoData[i].phases) {
        indexes[chn] = -1;
      }
      else if (indexes[chn] != -1) {
        indexes[chn] = i+1;
      }
      for (uint8_t j=chn+1; j<NUM_STICKS; j++) {
        indexes[j] = i+1;
      }
      g_model.expoData[i].srcRaw = MIXSRC_Rud+chn;
      g_model.expoData[i].chn = chn;
      g_model.expoData[i].swtch = ConvertSwitch_215_to_216(oldModel.expoData[i].swtch);
      g_model.expoData[i].phases = oldModel.expoData[i].phases;
      g_model.expoData[i].weight = oldModel.expoData[i].weight;
      memcpy(&g_model.expoData[i].name, &oldModel.expoData[i].name, LEN_EXPOMIX_NAME);
      if (oldModel.expoData[i].curveMode==0/*expo*/) {
        g_model.expoData[i].curve.type = CURVE_REF_EXPO;
        g_model.expoData[i].curve.value = oldModel.expoData[i].curveParam;
      }
      else if (oldModel.expoData[i].curveParam <= 6) {
        g_model.expoData[i].curve.type = CURVE_REF_FUNC;
        g_model.expoData[i].curve.value = oldModel.expoData[i].curveParam;
      }
      else {
        g_model.expoData[i].curve.type = CURVE_REF_CUSTOM;
        g_model.expoData[i].curve.value = oldModel.expoData[i].curveParam - 7;
      }
#else
      memcpy(&g_model.expoData[i], &oldModel.expoData[i], sizeof(g_model.expoData[i]));
#endif
    }
    else {
      break;
    }
  }
#if defined(PCBTARANIS)
  for (int i=NUM_STICKS-1; i>=0; i--) {
    int idx = indexes[i];
    if (idx >= 0) {
      ExpoData *expo = expoAddress(idx);
      memmove(expo+1, expo, (MAX_EXPOS-(idx+1))*sizeof(ExpoData));
      memclear(expo, sizeof(ExpoData));
      expo->srcRaw = MIXSRC_Rud + i;
      expo->chn = i;
      expo->weight = 100;
      expo->curve.type = CURVE_REF_EXPO;
    }
    for (int c=0; c<4; c++) {
      g_model.inputNames[i][c] = char2idx(STR_VSRCRAW[1+STR_VSRCRAW[0]*(i+1)+c]);
    }
  }
#endif
  for (uint8_t i=0; i<16; i++) {
#if defined(PCBTARANIS)
    int8_t *cur = &oldModel.points[i==0 ? 0 : 5*i+oldModel.curves[i-1]];
    int8_t *nxt = &oldModel.points[5+5*i+oldModel.curves[i]];
    uint8_t size = nxt - cur;
    if ((size & 1) == 0) {
      g_model.curves[i].type = CURVE_TYPE_CUSTOM;
      g_model.curves[i].points = (size / 2) - 4;
    }
    else {
      g_model.curves[i].points = size-5;
    }
#else
    g_model.curves[i] = oldModel.curves[i];
#endif
  }
  for (uint16_t i=0; i<512; i++) {
    g_model.points[i] = oldModel.points[i];
  }
  for (uint8_t i=0; i<32; i++) {
    LogicalSwitchData & sw = g_model.customSw[i];
    sw.func = oldModel.customSw[i].func;
    if (sw.func >= LS_FUNC_RANGE) sw.func += 1;
    if (sw.func >= LS_FUNC_STAY) sw.func += 1;
    sw.v1 = oldModel.customSw[i].v1;
    sw.v2 = oldModel.customSw[i].v2;
    sw.delay = oldModel.customSw[i].delay;
    sw.duration = oldModel.customSw[i].duration;
    sw.andsw = ConvertSwitch_215_to_216(oldModel.customSw[i].andsw);
#if defined(PCBTARANIS)
    uint8_t cstate = cswFamily(sw.func);
    if (cstate == LS_FAMILY_BOOL) {
      sw.v1 = ConvertSwitch_215_to_216(sw.v1);
      sw.v2 = ConvertSwitch_215_to_216(sw.v2);
    }
    else if (cstate == LS_FAMILY_OFS || cstate == LS_FAMILY_COMP || cstate == LS_FAMILY_DIFF) {
      sw.v1 = ConvertSource_215_to_216(sw.v1);
      if (cstate == LS_FAMILY_OFS || cstate == LS_FAMILY_DIFF) {
        if ((uint8_t)sw.v1 >= MIXSRC_FIRST_TELEM) {
          switch ((uint8_t)sw.v1) {
            case MIXSRC_FIRST_TELEM + TELEM_TM1-1:
            case MIXSRC_FIRST_TELEM + TELEM_TM2-1:
              sw.v2 = (sw.v2+128) * 3;
              break;
            case MIXSRC_FIRST_TELEM + TELEM_ALT-1:
            case MIXSRC_FIRST_TELEM + TELEM_GPSALT-1:
            case MIXSRC_FIRST_TELEM + TELEM_MIN_ALT-1:
            case MIXSRC_FIRST_TELEM + TELEM_MAX_ALT-1:
              sw.v2 = (sw.v2+128) * 8 - 500;
              break;
            case MIXSRC_FIRST_TELEM + TELEM_RPM-1:
            case MIXSRC_FIRST_TELEM + TELEM_MAX_RPM-1:
              sw.v2 = (sw.v2+128) * 50;
              break;
            case MIXSRC_FIRST_TELEM + TELEM_T1-1:
            case MIXSRC_FIRST_TELEM + TELEM_T2-1:
            case MIXSRC_FIRST_TELEM + TELEM_MAX_T1-1:
            case MIXSRC_FIRST_TELEM + TELEM_MAX_T2-1:
              sw.v2 = (sw.v2+128) + 30;
              break;
            case MIXSRC_FIRST_TELEM + TELEM_CELL-1:
            case MIXSRC_FIRST_TELEM + TELEM_HDG-1:
              sw.v2 = (sw.v2+128) * 2;
              break;
            case MIXSRC_FIRST_TELEM + TELEM_DIST-1:
            case MIXSRC_FIRST_TELEM + TELEM_MAX_DIST-1:
              sw.v2 = (sw.v2+128) * 8;
              break;
            case MIXSRC_FIRST_TELEM + TELEM_CURRENT-1:
            case MIXSRC_FIRST_TELEM + TELEM_POWER-1:
              sw.v2 = (sw.v2+128) * 5;
              break;
            case MIXSRC_FIRST_TELEM + TELEM_CONSUMPTION-1:
              sw.v2 = (sw.v2+128) * 20;
              break;
            default:
              sw.v2 += 128;
              break;
          }
        }
      }
      else if (cstate == LS_FAMILY_COMP) {
        sw.v2 = ConvertSource_215_to_216(sw.v2);
      }
    }
#endif
  }
  for (uint8_t i=0; i<32; i++) {
    CustomFnData & fn = g_model.funcSw[i];
    fn.swtch = ConvertSwitch_215_to_216(oldModel.funcSw[i].swtch);
    fn.func = oldModel.funcSw[i].func;
    if (fn.func <= 15) {
      fn.all.param = fn.func;
      fn.func = FUNC_SAFETY_CHANNEL;
    }
    else if (fn.func <= 20) {
      fn.all.param = fn.func - 16;
      fn.func = FUNC_TRAINER;
    }
    else if (fn.func == 21) {
      fn.func = FUNC_INSTANT_TRIM;
    }
    else if (fn.func == 22) {
      fn.func = FUNC_PLAY_SOUND;
    }
#if defined(PCBSKY9X)
    else if (fn.func == 23) {
      fn.func = FUNC_HAPTIC;
    }
#endif
    else if (fn.func == 23+IS_PCBSKY9X) {
      fn.func = FUNC_RESET;
    }
    else if (fn.func == 24+IS_PCBSKY9X) {
      fn.func = FUNC_VARIO;
    }
    else if (fn.func == 25+IS_PCBSKY9X) {
      fn.func = FUNC_PLAY_TRACK;
    }
    else if (fn.func == 26+IS_PCBSKY9X) {
      fn.func = FUNC_PLAY_VALUE;
    }
    else if (fn.func == 27+IS_PCBSKY9X) {
      fn.func = FUNC_LOGS;
    }
    else if (fn.func == 28+IS_PCBSKY9X) {
      fn.func = FUNC_VOLUME;
    }
    else if (fn.func == 29+IS_PCBSKY9X) {
      fn.func = FUNC_BACKLIGHT;
    }
    else if (fn.func == 30+IS_PCBSKY9X) {
      fn.func = FUNC_BACKGND_MUSIC;
    }
    else if (fn.func == 31+IS_PCBSKY9X) {
      fn.func = FUNC_BACKGND_MUSIC_PAUSE;
    }
    else {
      fn.all.param = fn.func - 32 - IS_PCBSKY9X;
      fn.all.mode = oldModel.funcSw[i].mode;
      fn.func = FUNC_ADJUST_GVAR;
    }

    fn.active = oldModel.funcSw[i].active;
    if (HAS_REPEAT_PARAM(fn.func)) {
      fn.active *= 5;
    }

    if (fn.func == FUNC_PLAY_TRACK || fn.func == FUNC_BACKGND_MUSIC) {
      memcpy(fn.play.name, oldModel.funcSw[i].param.name, LEN_CFN_NAME);
    }
    else {
      fn.all.val = oldModel.funcSw[i].param.composite.val;
    }
    if (fn.func == FUNC_PLAY_VALUE || fn.func == FUNC_VOLUME || (IS_ADJUST_GV_FUNC(fn.func) && fn.all.mode == FUNC_ADJUST_GVAR_SOURCE)) {
#if defined(PCBTARANIS)
      fn.all.val = ConvertSource_215_to_216(fn.all.val, true);
#endif
    }
  }

  g_model.swashR = oldModel.swashR;
  g_model.swashR.collectiveSource = ConvertSource_215_to_216(g_model.swashR.collectiveSource);

  for (uint8_t i=0; i<9; i++) {
    memcpy(&g_model.phaseData[i], &oldModel.phaseData[i], sizeof(oldModel.phaseData[i])); // the last 4 gvars will remain blank
#if defined(PCBTARANIS)
    for (uint8_t t=0; t<4; t++) {
      int trim = oldModel.phaseData[i].trim[t];
      if (trim > 500) {
        trim -= 501;
        if (trim >= i)
          trim += 1;
        g_model.phaseData[i].trim[t].mode = 2*trim;
        g_model.phaseData[i].trim[t].value = 0;
      }
      else {
        g_model.phaseData[i].trim[t].mode = 2*i;
        g_model.phaseData[i].trim[t].value = trim;
      }
    }
#endif
  }
  g_model.thrTraceSrc = oldModel.thrTraceSrc;
  g_model.switchWarningStates = oldModel.switchWarningStates >> 1;
  g_model.nSwToWarn = (oldModel.switchWarningStates & 0x01) ? 0xFF : 0;
  for (uint8_t i=0; i<5; i++) {
    memcpy(g_model.gvars[i].name, oldModel.gvar_names[i], LEN_GVAR_NAME);
  }

  memcpy(&g_model.frsky, &oldModel.frsky, sizeof(oldModel.frsky));

#if defined(PCBTARANIS)
  g_model.externalModule = oldModel.externalModule;
  g_model.trainerMode = oldModel.trainerMode;
  memcpy(g_model.curveNames, oldModel.curveNames, sizeof(g_model.curveNames));
#endif

  memcpy(g_model.moduleData, oldModel.moduleData, sizeof(g_model.moduleData));
}

bool eeConvert()
{
  const char *msg = NULL;

  if (g_eeGeneral.version == 215) {
    msg = PSTR("EEprom Data v215");
  }
  else {
    return false;
  }

  int conversionVersionStart = g_eeGeneral.version;

  // Information to the user and wait for key press
  g_eeGeneral.optrexDisplay = 0;
  g_eeGeneral.backlightMode = e_backlight_mode_on;
  g_eeGeneral.backlightBright = 0;
  g_eeGeneral.contrast = 25;
  ALERT(STR_EEPROMWARN, msg, AU_BAD_EEPROM);

  // Message
  MESSAGE(STR_EEPROMWARN, PSTR("EEPROM Converting"), NULL, AU_EEPROM_FORMATTING); // TODO translations

  // General Settings conversion
#if defined(PCBTARANIS)
  theFile.openRlc(0);
  theFile.readRlc((uint8_t*)&g_eeGeneral, sizeof(g_eeGeneral));
#else
  #pragma message("TODO openRlc for sky9x")
#endif
  if (g_eeGeneral.version == 215) ConvertGeneralSettings_215_to_216(g_eeGeneral);
  s_eeDirtyMsk = EE_GENERAL;
  eeCheck(true);

  lcd_rect(60, 6*FH+4, 132, 3);

  // Models conversion
  uint8_t currModel = g_eeGeneral.currModel;
  for (uint8_t id=0; id<MAX_MODELS; id++) {
    lcd_hline(61, 6*FH+5, 10+id*2, FORCE);
    // lcd_putsnAtt(61, 7*FH)
    lcdRefresh();
    if (eeModelExists(id)) {
      // TODO loadModel without anything else
#if defined(PCBTARANIS)
      theFile.openRlc(FILE_MODEL(id));
      theFile.readRlc((uint8_t*)&g_model, sizeof(g_model));
#else
      #pragma message("TODO openRlc for sky9x")
#endif
      int version = conversionVersionStart;
      if (version == 215) {
        version = 216;
        ConvertModel_215_to_216(g_model);
      }
      g_eeGeneral.currModel = id;
      s_eeDirtyMsk = EE_MODEL;
      eeCheck(true);
    }

  }
  g_eeGeneral.currModel = currModel;

  return true;
}
