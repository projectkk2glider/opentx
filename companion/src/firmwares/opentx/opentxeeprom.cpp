#include <stdlib.h>
#include <algorithm>
#include "helpers.h"
#include "opentxeeprom.h"
#include <QObject>

#define IS_DBLEEPROM(board, version)         ((board==BOARD_GRUVIN9X || board==BOARD_M128) && version >= 213)
// Macro used for Gruvin9x board and M128 board between versions 213 and 214 (when there were stack overflows!)
#define IS_DBLRAM(board, version)            ((board==BOARD_GRUVIN9X && version >= 213) || (board==BOARD_M128 && version >= 213 && version <= 214))

#define HAS_PERSISTENT_TIMERS(board)         (IS_ARM(board) || board == BOARD_GRUVIN9X)
#define HAS_LARGE_LCD(board)                 IS_TARANIS(board)
#define MAX_VIEWS(board)                     (HAS_LARGE_LCD(board) ? 2 : 256)
#define MAX_POTS(board)                      (IS_TARANIS(board) ? 4 : 3)
#define MAX_SWITCHES(board)                  (IS_TARANIS(board) ? 8 : 7)
#define MAX_SWITCHES_POSITION(board)         (IS_TARANIS(board) ? 22 : 9)
#define MAX_ROTARY_ENCODERS(board)           (board==BOARD_GRUVIN9X ? 2 : (board==BOARD_SKY9X ? 1 : 0))
#define MAX_PHASES(board, version)           (IS_ARM(board) ? 9 :  (IS_DBLRAM(board, version) ? 6 :  5))
#define MAX_MIXERS(board, version)           (IS_ARM(board) ? 64 : 32)
#define MAX_CHANNELS(board, version)         (IS_ARM(board) ? 32 : 16)
#define MAX_EXPOS(board, version)            (IS_ARM(board) ? ((IS_TARANIS(board) && version >= 216) ? 64 : 32) : (IS_DBLRAM(board, version) ? 16 : 14))
#define MAX_CUSTOM_SWITCHES(board, version)  (IS_ARM(board) ? 32 : (IS_DBLEEPROM(board, version) ? 15 : 12))
#define MAX_CUSTOM_FUNCTIONS(board, version) (IS_ARM(board) ? 32 : (IS_DBLEEPROM(board, version) ? 24 : 16))
#define MAX_CURVES(board, version)           (IS_ARM(board) ? ((IS_TARANIS(board) && version >= 216) ? 32 : 16) : O9X_MAX_CURVES)
#define MAX_GVARS(board, version)            ((IS_ARM(board) && version >= 216) ? 9 : 5)

#define IS_AFTER_RELEASE_21_MARCH_2013(board, version) (version >= 214 || (!IS_ARM(board) && version >= 213))
#define IS_AFTER_RELEASE_23_MARCH_2013(board, version) (version >= 214 || (board==BOARD_STOCK && version >= 213))

inline int switchIndex(int i, BoardEnum board, unsigned int version)
{
  bool afterrelease21March2013 = IS_AFTER_RELEASE_21_MARCH_2013(board, version);
  if (!IS_TARANIS(board) && afterrelease21March2013)
    return (i<=3 ? i+3 : (i<=6 ? i-3 : i));
  else
    return i;
}

class SwitchesConversionTable: public ConversionTable {

  public:
    SwitchesConversionTable(BoardEnum board, unsigned int version, unsigned long flags=0)
    {
      int val=0;
      int offset=0;
      if (flags & POPULATE_TIMER_MODES) {
        offset = 4;
        for (int i=0; i<5; i++) {
          addConversion(RawSwitch(SWITCH_TYPE_TIMER_MODE, i), val++);
        }
      }
      else {
        addConversion(RawSwitch(SWITCH_TYPE_NONE), val++);
      }

      for (int i=1; i<=MAX_SWITCHES_POSITION(board); i++) {
        int s = switchIndex(i, board, version);
        addConversion(RawSwitch(SWITCH_TYPE_SWITCH, -s), -val+offset);
        addConversion(RawSwitch(SWITCH_TYPE_SWITCH, s), val++);
      }

      if (version >= 216) {
        for (int i=1; i<=GetEepromInterface()->getCapability(MultiposPots) * GetEepromInterface()->getCapability(MultiposPotsPositions); i++) {
          addConversion(RawSwitch(SWITCH_TYPE_MULTIPOS_POT, -i), -val+offset);
          addConversion(RawSwitch(SWITCH_TYPE_MULTIPOS_POT, i), val++);
        }
      }

      if (version >= 216) {
        for (int i=1; i<=8; i++) {
          addConversion(RawSwitch(SWITCH_TYPE_TRIM, -i), -val+offset);
          addConversion(RawSwitch(SWITCH_TYPE_TRIM, i), val++);
        }
      }

      if (version >= 216) {
        for (int i=1; i<=MAX_ROTARY_ENCODERS(board); i++) {
          addConversion(RawSwitch(SWITCH_TYPE_ROTARY_ENCODER, -i), -val+offset);
          addConversion(RawSwitch(SWITCH_TYPE_ROTARY_ENCODER, i), val++);
        }
      }

      for (int i=1; i<=MAX_CUSTOM_SWITCHES(board, version); i++) {
        addConversion(RawSwitch(SWITCH_TYPE_VIRTUAL, -i), -val+offset);
        addConversion(RawSwitch(SWITCH_TYPE_VIRTUAL, i), val++);
      }

      addConversion(RawSwitch(SWITCH_TYPE_OFF), -val+offset);
      addConversion(RawSwitch(SWITCH_TYPE_ON), val++);

      if (version < 216) {
        // previous "moment" switches
        for (int i=1; i<=MAX_SWITCHES_POSITION(board); i++) {
          int s = switchIndex(i, board, version);
          addConversion(RawSwitch(SWITCH_TYPE_SWITCH, s), val++);
        }

        for (int i=1; i<=MAX_CUSTOM_SWITCHES(board, version); i++) {
          addConversion(RawSwitch(SWITCH_TYPE_VIRTUAL, i), val++);
        }

        // previous "One" switch
        addConversion(RawSwitch(SWITCH_TYPE_ON), val++);
      }
    }

  protected:

    void addConversion(const RawSwitch & sw, const int b)
    {
      ConversionTable::addConversion(sw.toValue(), b);
    }

    class Cache {
      public:
        Cache(BoardEnum board, unsigned int version, unsigned long flags, SwitchesConversionTable * table):
          board(board),
          version(version),
          flags(flags),
          table(table)
        {
        }
        BoardEnum board;
        unsigned int version;
        unsigned long flags;
        SwitchesConversionTable * table;
    };

  public:

    static SwitchesConversionTable * getInstance(BoardEnum board, unsigned int version, unsigned long flags=0)
    {
      static std::list<Cache> internalCache;

      for (std::list<Cache>::iterator it=internalCache.begin(); it!=internalCache.end(); it++) {
        Cache element = *it;
        if (element.board == board && element.version == version && element.flags == flags)
          return element.table;
      }

      Cache element(board, version, flags, new SwitchesConversionTable(board, version, flags));
      internalCache.push_back(element);
      return element.table;
    }
};

#define FLAG_NONONE       0x01
#define FLAG_NOSWITCHES   0x02
#define FLAG_NOTELEMETRY  0x04

class SourcesConversionTable: public ConversionTable {

  public:
    SourcesConversionTable(BoardEnum board, unsigned int version, unsigned int variant, unsigned long flags=0)
    {
      bool afterrelease21March2013 = IS_AFTER_RELEASE_21_MARCH_2013(board, version);

      int val=0;

      if (!(flags & FLAG_NONONE)) {
        addConversion(RawSource(SOURCE_TYPE_NONE), val++);
      }

      if (IS_TARANIS(board) && version >= 216) {
        for (int i=0; i<32; i++)
          addConversion(RawSource(SOURCE_TYPE_VIRTUAL_INPUT, i), val++);
        for (int i=0; i<3; i++) {
          for (int j=0; j<6; j++) {
            addConversion(RawSource(SOURCE_TYPE_LUA_INPUT, i*16+j), val++);
          }
        }
      }

      for (int i=0; i<4+MAX_POTS(board); i++)
        addConversion(RawSource(SOURCE_TYPE_STICK, i), val++);

      for (int i=0; i<MAX_ROTARY_ENCODERS(board); i++)
        addConversion(RawSource(SOURCE_TYPE_ROTARY_ENCODER, 0), val++);

      if (!afterrelease21March2013) {
        for (int i=0; i<NUM_STICKS; i++)
          addConversion(RawSource(SOURCE_TYPE_TRIM, i), val++);
      }

      addConversion(RawSource(SOURCE_TYPE_MAX), val++);

      if (afterrelease21March2013) {
        for (int i=0; i<3; i++)
          addConversion(RawSource(SOURCE_TYPE_CYC, i), val++);
      }

      if (afterrelease21March2013) {
        for (int i=0; i<NUM_STICKS; i++)
          addConversion(RawSource(SOURCE_TYPE_TRIM, i), val++);
      }

      addConversion(RawSource(SOURCE_TYPE_SWITCH, 0), val++);

      if (!(flags & FLAG_NOSWITCHES)) {
        if (afterrelease21March2013) {
          for (int i=1; i<MAX_SWITCHES(board); i++)
            addConversion(RawSource(SOURCE_TYPE_SWITCH, i), val++);
        }
        else {
          for (int i=1; i<=9; i++) {
            if (i>=4 && i<=6)
              addConversion(RawSource(SOURCE_TYPE_SWITCH, 0), val++);
            else
              addConversion(RawSource(SOURCE_TYPE_SWITCH, i), val++);
          }
        }
        for (int i=0; i<MAX_CUSTOM_SWITCHES(board, version); i++)
          addConversion(RawSource(SOURCE_TYPE_CUSTOM_SWITCH, i), val++);
      }

      if (!afterrelease21March2013) {
        for (int i=0; i<3; i++)
          addConversion(RawSource(SOURCE_TYPE_CYC, i), val++);
      }

      for (int i=0; i<8; i++)
        addConversion(RawSource(SOURCE_TYPE_PPM, i), val++);

      for (int i=0; i<MAX_CHANNELS(board, version); i++)
        addConversion(RawSource(SOURCE_TYPE_CH, i), val++);

      if (!(flags & FLAG_NOTELEMETRY)) {
        if (afterrelease21March2013) {
          if ((board != BOARD_STOCK && (board!=BOARD_M128 || version<215)) || (variant & GVARS_VARIANT)) {
            for (int i=0; i<MAX_GVARS(board, version); i++)
              addConversion(RawSource(SOURCE_TYPE_GVAR, i), val++);
          }
        }

        if (afterrelease21March2013)
          addConversion(RawSource(SOURCE_TYPE_TELEMETRY, 0), val++);

        for (int i=1; i<TELEMETRY_SOURCE_ACC; i++) {
          if (version < 216) {
            if (i==TELEMETRY_SOURCE_ASPD-1 || i==TELEMETRY_SOURCE_DTE-1 || i==TELEMETRY_SOURCE_CELL_MIN-1 || i==TELEMETRY_SOURCE_VFAS_MIN-1)
              continue;
          }
          addConversion(RawSource(SOURCE_TYPE_TELEMETRY, i), val++);
        }
      }
    }

  protected:

    void addConversion(const RawSource & source, const int b)
    {
      ConversionTable::addConversion(source.toValue(), b);
    }

    class Cache {
      public:
        Cache(BoardEnum board, unsigned int version, unsigned int variant, unsigned long flags, SourcesConversionTable * table):
          board(board),
          version(version),
          variant(variant),
          flags(flags),
          table(table)
        {
        }
        BoardEnum board;
        unsigned int version;
        unsigned int variant;
        unsigned long flags;
        SourcesConversionTable * table;
    };

  public:

    static SourcesConversionTable * getInstance(BoardEnum board, unsigned int version, unsigned int variant, unsigned long flags=0)
    {
      static std::list<Cache> internalCache;

      for (std::list<Cache>::iterator it=internalCache.begin(); it!=internalCache.end(); it++) {
        Cache element = *it;
        if (element.board == board && element.version == version && element.variant == variant && element.flags == flags)
          return element.table;
      }

      Cache element(board, version, variant, flags, new SourcesConversionTable(board, version, variant, flags));
      internalCache.push_back(element);
      return element.table;
    }
};

template <int N>
class SwitchField: public ConversionField< SignedField<N> > {
  public:
    SwitchField(RawSwitch & sw, BoardEnum board, unsigned int version, unsigned long flags=0):
      ConversionField< SignedField<N> >(_switch, SwitchesConversionTable::getInstance(board, version, flags), "Switch"),
      sw(sw),
      _switch(0)
    {
    }

    virtual void beforeExport()
    {
      _switch = sw.toValue();
      ConversionField< SignedField<N> >::beforeExport();
    }
    
    virtual void afterImport()
    {
      ConversionField< SignedField<N> >::afterImport();	
      sw = RawSwitch(_switch);
    }    
    
  protected:
    RawSwitch & sw;
    int _switch;
};

template <int N>
class SourceField: public ConversionField< UnsignedField<N> > {
  public:
    SourceField(RawSource & source, BoardEnum board, unsigned int version, unsigned int variant, unsigned long flags=0):
      ConversionField< UnsignedField<N> >(_source, SourcesConversionTable::getInstance(board, version, variant, flags), "Source"),
      source(source),
      _source(0)
    {
    }

    virtual void beforeExport()
    {
      _source = source.toValue();
      ConversionField< UnsignedField<N> >::beforeExport();
    }
    
    virtual void afterImport()
    {
      ConversionField< UnsignedField<N> >::afterImport();	
      source = RawSource(_source);
    }    

  protected:
    RawSource & source;
    unsigned int _source;
};

class CurveReferenceField: public TransformedField {
  public:
    CurveReferenceField(CurveReference & curve, BoardEnum board, unsigned int version):
      TransformedField(internalField),
      curve(curve),
      _curve_type(0)
    {
      internalField.Append(new UnsignedField<8>(_curve_type));
      internalField.Append(new SignedField<8>(curve.value));
    }

    virtual void beforeExport()
    {
      if (curve.value != 0)
        _curve_type = (unsigned int)curve.type;
      else
        _curve_type = 0;
    }

    virtual void afterImport()
    {
      curve.type = (CurveReference::CurveRefType)_curve_type;
    }

  protected:
    StructField internalField;
    CurveReference & curve;
    unsigned int _curve_type;
};

class HeliField: public StructField {
  public:
    HeliField(SwashRingData & heli, BoardEnum board, unsigned int version, unsigned int variant)
    {
      Append(new BoolField<1>(heli.invertELE));
      Append(new BoolField<1>(heli.invertAIL));
      Append(new BoolField<1>(heli.invertCOL));
      Append(new UnsignedField<5>(heli.type));
      Append(new SourceField<8>(heli.collectiveSource, board, version, variant));
      //, FLAG_NOSWITCHES)); Fix shift in collective
      Append(new UnsignedField<8>(heli.value));
    }
};

class PhaseField: public TransformedField {
  public:
    PhaseField(PhaseData & phase, int index, BoardEnum board, unsigned int version):
      TransformedField(internalField),
      internalField("Phase"),
      phase(phase),
      index(index),
      board(board),
      version(version),
      rotencCount(IS_ARM(board) ? 1 : (board == BOARD_GRUVIN9X ? 2 : 0))
    {
      if (board == BOARD_STOCK || (board==BOARD_M128 && version>=215)) {
        // On stock we use 10bits per trim
        for (int i=0; i<NUM_STICKS; i++)
          internalField.Append(new SignedField<8>(trimBase[i]));
        for (int i=0; i<NUM_STICKS; i++)
          internalField.Append(new SignedField<2>(trimExt[i]));
      }
      else if (board == BOARD_TARANIS && version >= 216) {
        for (int i=0; i<NUM_STICKS; i++) {
          internalField.Append(new SignedField<11>(phase.trim[i]));
          internalField.Append(new UnsignedField<5>(trimMode[i]));
        }
      }
      else {
        for (int i=0; i<NUM_STICKS; i++)
          internalField.Append(new SignedField<16>(trimBase[i]));
      }

      internalField.Append(new SwitchField<8>(phase.swtch, board, version));
      if (HAS_LARGE_LCD(board))
        internalField.Append(new ZCharField<10>(phase.name));
      else
        internalField.Append(new ZCharField<6>(phase.name));

      if (IS_ARM(board) && version >= 214) {
        internalField.Append(new UnsignedField<8>(phase.fadeIn));
        internalField.Append(new UnsignedField<8>(phase.fadeOut));
      }
      else {
        internalField.Append(new UnsignedField<4>(phase.fadeIn));
        internalField.Append(new UnsignedField<4>(phase.fadeOut));
      }

      for (int i=0; i<rotencCount; i++) {
        internalField.Append(new SignedField<16>(phase.rotaryEncoders[i]));
      }

      if (board != BOARD_STOCK && (board != BOARD_M128 || version < 215)) {
        for (int i=0; i<MAX_GVARS(board, version); i++) {
          internalField.Append(new SignedField<16>(phase.gvars[i]));
        }
      }
    }

    virtual void beforeExport()
    {
      for (int i=0; i<NUM_STICKS; i++) {
        if (board == BOARD_TARANIS && version >= 216) {
          if (phase.trimMode[i] < 0)
            trimMode[i] = TRIM_MODE_NONE;
          else
            trimMode[i] = 2*phase.trimRef[i] + phase.trimMode[i];
        }
        else {
          int trim;
          if (phase.trimMode[i] < 0)
            trim = 0;
          else if (phase.trimRef[i] != index)
            trim = 501 + phase.trimRef[i] - (phase.trimRef[i] > index ? 1 : 0);
          else
            trim = std::max(-500, std::min(500, phase.trim[i]));
          if (board == BOARD_STOCK || (board == BOARD_M128 && version >= 215)) {
            trimBase[i] = trim >> 2;
            trimExt[i] = (trim & 0x03);
          }
          else {
            trimBase[i] = trim;
          }
        }
      }
    }

    virtual void afterImport()
    {
      for (int i=0; i<NUM_STICKS; i++) {
        if (board == BOARD_TARANIS && version >= 216) {
          if (trimMode[i] == TRIM_MODE_NONE) {
            phase.trimMode[i] = -1;
          }
          else {
            phase.trimMode[i] = trimMode[i] % 2;
            phase.trimRef[i] = trimMode[i] / 2;
          }
        }
        else {
          int trim;
          if (board == BOARD_STOCK || (board == BOARD_M128 && version >= 215))
            trim = ((trimBase[i]) << 2) + (trimExt[i] & 0x03);
          else
            trim = trimBase[i];
          if (trim > 500) {
            phase.trimRef[i] = trim - 501;
            if (phase.trimRef[i] >= index)
              phase.trimRef[i] += 1;
            phase.trim[i] = 0;
          }
          else {
            phase.trim[i] = trim;
          }
        }
      }
    }

  protected:
    StructField internalField;
    PhaseData & phase;
    int index;
    BoardEnum board;
    unsigned int version;
    int rotencCount;
    int trimBase[NUM_STICKS];
    int trimExt[NUM_STICKS];
    unsigned int trimMode[NUM_STICKS];
};


int smallGvarToEEPROM(int gvar)
{
  if (gvar < -10000) {
    gvar = 128 + gvar + 10000;
  }
  else if (gvar > 10000) {
    gvar = -128 +gvar - 10001;
  }
  return gvar;
}

int smallGvarToC9x(int gvar)
{
  if (gvar>110) {
    gvar = gvar-128 - 10000;
  }
  else if (gvar<-110) {
    gvar = gvar+128 + 10001;
  }
  return gvar;
}


void splitGvarParam(const int gvar, int & _gvar, unsigned int & _gvarParam, const BoardEnum board, const int version)
{
  if (version >= 214 || (!IS_ARM(board) && version >= 213)) {
    if (gvar < -10000) {
      _gvarParam = 0;
      _gvar = 256 + gvar + 10000;
    }
    else if (gvar > 10000) {
      _gvarParam = 1;
      _gvar = gvar - 10001;
    }
    else {
      if (gvar < 0) _gvarParam = 1;
      else          _gvarParam = 0;
      _gvar = gvar;  // save routine throws away all unused bits; therefore no 2er complement compensation needed here
    }
  }
  else {
    if (gvar < -10000) {
      _gvarParam = 1;
      _gvar = gvar + 10000;
    }
    else if (gvar > 10000) {
      _gvarParam = 1;
      _gvar = gvar - 10001;
    }
    else {
      _gvarParam = 0;
      _gvar = gvar;
    }
  }
}

void concatGvarParam(int & gvar, const int _gvar, const unsigned int _gvarParam, const BoardEnum board, const int version)
{
  if (version >= 214 || (!IS_ARM(board) && version >= 213)) {
	gvar = _gvar;
    if (gvar<0) gvar+=256;  // remove 2er complement, because 8bit part is in this case unsigned
	if (_gvarParam) {  // here is the real sign bit
	  gvar|=-256;   // set all higher bits to simulate negative value
	}

    if (gvar>245) {
        gvar = gvar-256 - 10000;
    } else if (gvar<-245) {
        gvar = gvar+256 + 10001;
    }
  }
  else {
    if (_gvarParam == 0) {
      gvar = _gvar;
    }
    else if (_gvar >= 0) {
      gvar = 10001 + _gvar;
    }
    else {
      gvar = -10000 + _gvar;
    }
  }
}

void exportGvarParam(const int gvar, int & _gvar)
{
  if (gvar < -10000) {
    _gvar = 512 + gvar + 10000;
  }
  else if (gvar > 10000) {
    _gvar = 512 + gvar - 10001;
  }
  else {
    _gvar = gvar;
  }
}

void importGvarParam(int & gvar, const int _gvar)
{
  if (_gvar >= 512) {
    gvar = 10001 + _gvar - 512;
  }
  else if (_gvar >= 512-5) {
    gvar = -10000 + _gvar - 512;
  }
  else if (_gvar < -512) {
    gvar = -10000 + _gvar + 513;
  }
  else if (_gvar < -512+5) {
    gvar = 10000 + _gvar + 513;
  }
  else {
    gvar = _gvar;
  }

  // qDebug() << QString("import") << _gvar << gvar;
}

class MixField: public TransformedField {
  public:
    MixField(MixData & mix, BoardEnum board, unsigned int version):
      TransformedField(internalField),
      internalField("Mix"),
      mix(mix),
      board(board),
      version(version)
    {
      if (IS_TARANIS(board) && version >= 216) {
        internalField.Append(new UnsignedField<8>(_destCh));
        internalField.Append(new UnsignedField<16>(mix.phases));
        internalField.Append(new UnsignedField<8>((unsigned int &)mix.mltpx));
        internalField.Append(new SignedField<16>(_weight));
        internalField.Append(new SwitchField<8>(mix.swtch, board, version));
        internalField.Append(new CurveReferenceField(mix.curve, board, version));
        internalField.Append(new UnsignedField<4>(mix.mixWarn));
        internalField.Append(new UnsignedField<4>(mix.srcVariant));
        internalField.Append(new UnsignedField<8>(mix.delayUp));
        internalField.Append(new UnsignedField<8>(mix.delayDown));
        internalField.Append(new UnsignedField<8>(mix.speedUp));
        internalField.Append(new UnsignedField<8>(mix.speedDown));
        internalField.Append(new SourceField<8>(mix.srcRaw, board, version, FLAG_NOTELEMETRY));
        internalField.Append(new SignedField<16>(_offset));
        internalField.Append(new ZCharField<8>(mix.name));
        internalField.Append(new SpareBitsField<8>());
      }
      else if (IS_ARM(board)) {
        internalField.Append(new UnsignedField<8>(_destCh));
        internalField.Append(new UnsignedField<16>(mix.phases));
        internalField.Append(new BoolField<1>(_curveMode));
        internalField.Append(new BoolField<1>(mix.noExpo));
        internalField.Append(new SignedField<3>(mix.carryTrim));
        internalField.Append(new UnsignedField<2>((unsigned int &)mix.mltpx));
        if (version >= 214)
          internalField.Append(new SpareBitsField<1>());
        else
          internalField.Append(new UnsignedField<1>(_offsetMode));
        internalField.Append(new SignedField<16>(_weight));
        internalField.Append(new SwitchField<8>(mix.swtch, board, version));
        internalField.Append(new SignedField<8>(_curveParam));
        if (version >= 214) {
          internalField.Append(new UnsignedField<4>(mix.mixWarn));
          internalField.Append(new UnsignedField<4>(mix.srcVariant));
        }
        else {
          internalField.Append(new UnsignedField<8>(mix.mixWarn));
        }
        internalField.Append(new UnsignedField<8>(mix.delayUp));
        internalField.Append(new UnsignedField<8>(mix.delayDown));
        internalField.Append(new UnsignedField<8>(mix.speedUp));
        internalField.Append(new UnsignedField<8>(mix.speedDown));
        internalField.Append(new SourceField<8>(mix.srcRaw, board, version, FLAG_NOTELEMETRY));
        if (version >= 214)
          internalField.Append(new SignedField<16>(_offset));
        else
          internalField.Append(new SignedField<8>(_offset));
        if (HAS_LARGE_LCD(board)) {
          internalField.Append(new ZCharField<8>(mix.name));
          internalField.Append(new SpareBitsField<16>());
        }
        else {
          internalField.Append(new ZCharField<6>(mix.name));
        }
      }
      else if (IS_DBLRAM(board, version) && IS_AFTER_RELEASE_23_MARCH_2013(board, version)) {
        internalField.Append(new UnsignedField<4>(_destCh));
        internalField.Append(new BoolField<1>(_curveMode));
        internalField.Append(new BoolField<1>(mix.noExpo));
        internalField.Append(new UnsignedField<1>(_weightMode));
        internalField.Append(new UnsignedField<1>(_offsetMode));
        internalField.Append(new SourceField<8>(mix.srcRaw, board, version, FLAG_NOTELEMETRY));
        internalField.Append(new SignedField<8>(_weight));
        internalField.Append(new SwitchField<8>(mix.swtch, board, version));
        internalField.Append(new UnsignedField<8>(mix.phases));
        internalField.Append(new UnsignedField<2>((unsigned int &)mix.mltpx));
        internalField.Append(new SignedField<3>(mix.carryTrim));
        internalField.Append(new UnsignedField<2>(mix.mixWarn));
        internalField.Append(new SpareBitsField<1>());
        internalField.Append(new UnsignedField<4>(mix.delayUp));
        internalField.Append(new UnsignedField<4>(mix.delayDown));
        internalField.Append(new UnsignedField<4>(mix.speedUp));
        internalField.Append(new UnsignedField<4>(mix.speedDown));
        internalField.Append(new SignedField<8>(_curveParam));
        internalField.Append(new SignedField<8>(_offset));
      }
      else {
        internalField.Append(new UnsignedField<4>(_destCh));
        internalField.Append(new BoolField<1>(_curveMode));
        internalField.Append(new BoolField<1>(mix.noExpo));
        internalField.Append(new UnsignedField<1>(_weightMode));
        internalField.Append(new UnsignedField<1>(_offsetMode));
        internalField.Append(new SignedField<8>(_weight));
        internalField.Append(new SwitchField<6>(mix.swtch, board, version));
        internalField.Append(new UnsignedField<2>((unsigned int &)mix.mltpx));
        internalField.Append(new UnsignedField<5>(mix.phases));
        internalField.Append(new SignedField<3>(mix.carryTrim));
        internalField.Append(new SourceField<6>(mix.srcRaw, board, version, FLAG_NOTELEMETRY));
        internalField.Append(new UnsignedField<2>(mix.mixWarn));
        internalField.Append(new UnsignedField<4>(mix.delayUp));
        internalField.Append(new UnsignedField<4>(mix.delayDown));
        internalField.Append(new UnsignedField<4>(mix.speedUp));
        internalField.Append(new UnsignedField<4>(mix.speedDown));
        internalField.Append(new SignedField<8>(_curveParam));
        internalField.Append(new SignedField<8>(_offset));
      }
    }

    virtual void beforeExport()
    {
      if (mix.destCh && mix.srcRaw.type != SOURCE_TYPE_NONE) {
        _destCh = mix.destCh - 1;
        if (mix.curve.type == CurveReference::CURVE_REF_CUSTOM) {
          _curveMode = true;
          _curveParam = 6 + mix.curve.value;
        }
        else if (mix.curve.type == CurveReference::CURVE_REF_FUNC) {
          _curveMode = true;
          _curveParam = mix.curve.value;
        }
        else if (mix.curve.type == CurveReference::CURVE_REF_DIFF) {
          _curveMode = 0;
          _curveParam = smallGvarToEEPROM(mix.curve.value);
        }
      }
      else {
        mix.clear();
        _destCh = 0;
        _curveMode = 0;
        _curveParam = 0;
      }

      if (IS_ARM(board)) {
        exportGvarParam(mix.weight, _weight);
        if (version >= 214)
          exportGvarParam(mix.sOffset, _offset);
        else
          splitGvarParam(mix.sOffset, _offset, _offsetMode, board, version);
      }
      else {
        splitGvarParam(mix.weight, _weight, _weightMode, board, version);
        splitGvarParam(mix.sOffset, _offset, _offsetMode, board, version);
      }
    }

    virtual void afterImport()
    {
      if (mix.srcRaw.type != SOURCE_TYPE_NONE) {
        mix.destCh = _destCh + 1;
        if (!IS_ARM(board) || version < 216) {
          if (!_curveMode)
            mix.curve = CurveReference(CurveReference::CURVE_REF_DIFF, smallGvarToC9x(_curveParam));
          else if (_curveParam > 6)
            mix.curve = CurveReference(CurveReference::CURVE_REF_CUSTOM, _curveParam-6);
          else
            mix.curve = CurveReference(CurveReference::CURVE_REF_FUNC, _curveParam);
        }
      }

      if (IS_ARM(board)) {
        importGvarParam(mix.weight, _weight);
        if (version >= 214)
          importGvarParam(mix.sOffset, _offset);
        else
          concatGvarParam(mix.sOffset, _offset, _offsetMode, board, version);
      }
      else {
        concatGvarParam(mix.weight, _weight, _weightMode, board, version);
        concatGvarParam(mix.sOffset, _offset, _offsetMode, board, version);
      }
    }

  protected:
    StructField internalField;
    MixData & mix;
    BoardEnum board;
    unsigned int version;
    unsigned int _destCh;
    bool _curveMode;
    int _curveParam;
    int _weight;
    int _offset;
    unsigned int _weightMode;
    unsigned int _offsetMode;
};

class ExpoField: public TransformedField {
  public:
    ExpoField(ExpoData & expo, BoardEnum board, unsigned int version):
      TransformedField(internalField),
      internalField("Expo"),
      expo(expo),
      board(board),
      version(version)
    {
      if (IS_TARANIS(board) && version >= 216) {
        internalField.Append(new SourceField<8>(expo.srcRaw, board, version, 0));
        internalField.Append(new UnsignedField<16>(expo.scale));
        internalField.Append(new UnsignedField<8>(expo.chn, "Channel"));
        internalField.Append(new SwitchField<8>(expo.swtch, board, version));
        internalField.Append(new UnsignedField<16>(expo.phases));
        internalField.Append(new SignedField<8>(_weight, "Weight"));
        internalField.Append(new SignedField<8>(expo.carryTrim));
        internalField.Append(new ZCharField<8>(expo.name));
        internalField.Append(new SignedField<8>(expo.offset, "Offset"));
        internalField.Append(new CurveReferenceField(expo.curve, board, version));
        internalField.Append(new SpareBitsField<8>());
      }
      else if (IS_ARM(board)) {
        internalField.Append(new UnsignedField<8>(expo.mode));
        internalField.Append(new UnsignedField<8>(expo.chn));
        internalField.Append(new SwitchField<8>(expo.swtch, board, version));
        internalField.Append(new UnsignedField<16>(expo.phases));
        internalField.Append(new SignedField<8>(_weight));
        internalField.Append(new BoolField<8>(_curveMode));
        if (HAS_LARGE_LCD(board)) {
          internalField.Append(new ZCharField<8>(expo.name));
          internalField.Append(new SpareBitsField<16>());
        }
        else {
          internalField.Append(new ZCharField<6>(expo.name));
        }
        internalField.Append(new SignedField<8>(_curveParam));
      }
      else if (IS_DBLRAM(board, version) && IS_AFTER_RELEASE_23_MARCH_2013(board, version)) {
        internalField.Append(new UnsignedField<2>(expo.mode));
        internalField.Append(new UnsignedField<2>(expo.chn));
        internalField.Append(new BoolField<1>(_curveMode));
        internalField.Append(new SpareBitsField<3>());
        internalField.Append(new UnsignedField<8>(expo.phases));
        internalField.Append(new SwitchField<8>(expo.swtch, board, version));
        internalField.Append(new SignedField<8>(_weight));
        internalField.Append(new SignedField<8>(_curveParam));
      }
      else {
        internalField.Append(new UnsignedField<2>(expo.mode));
        internalField.Append(new SwitchField<6>(expo.swtch, board, version));
        internalField.Append(new UnsignedField<2>(expo.chn));
        internalField.Append(new UnsignedField<5>(expo.phases));
        internalField.Append(new BoolField<1>(_curveMode));
        internalField.Append(new SignedField<8>(_weight));
        internalField.Append(new SignedField<8>(_curveParam));
      }
    }

    virtual void beforeExport()
    {
      _weight    = smallGvarToEEPROM(expo.weight);
      if (!IS_TARANIS(board) || version < 216) {
        if (expo.curve.type==CurveReference::CURVE_REF_FUNC && expo.curve.value) {
          _curveMode = true;
          _curveParam = expo.curve.value;
        }
        else if (expo.curve.type==CurveReference::CURVE_REF_CUSTOM && expo.curve.value) {
          _curveMode = true;
          _curveParam = expo.curve.value+6;
        }
        else {
          _curveMode = false;
          _curveParam = smallGvarToEEPROM(expo.curve.value);
        }
      }
    }

    virtual void afterImport()
    {
      expo.weight     = smallGvarToC9x(_weight);
      if (!IS_TARANIS(board) || version < 216) {
        if (!_curveMode)
          expo.curve = CurveReference(CurveReference::CURVE_REF_EXPO, smallGvarToC9x(_curveParam));
        else if (_curveParam > 6)
          expo.curve = CurveReference(CurveReference::CURVE_REF_CUSTOM, _curveParam-6);
        else
          expo.curve = CurveReference(CurveReference::CURVE_REF_FUNC, _curveParam);
      }
    }

  protected:
    StructField internalField;
    ExpoData & expo;
    BoardEnum board;
    unsigned int version;
    bool _curveMode;
    int  _weight;
    int  _curveParam;
};

class LimitField: public StructField {
  public:
    LimitField(LimitData & limit, BoardEnum board, unsigned int version):
      StructField("Limit")
    {
      if (IS_ARM(board) && version >= 216) {
        Append(new ConversionField< SignedField<16> >(limit.min, +1000));
        Append(new ConversionField< SignedField<16> >(limit.max, -1000));
      }
      else {
        Append(new ConversionField< SignedField<8> >(limit.min, +100, 10));
        Append(new ConversionField< SignedField<8> >(limit.max, -100, 10));
      }
      Append(new SignedField<8>(limit.ppmCenter));
      Append(new SignedField<14>(limit.offset));
      Append(new BoolField<1>(limit.symetrical));
      Append(new BoolField<1>(limit.revert));
      if (HAS_LARGE_LCD(board)) {
        Append(new ZCharField<6>(limit.name));
      }
      if (IS_TARANIS(board) && version >= 216) {
        Append(new SignedField<8>(limit.curve.value));
      }
    }
};

class CurvesField: public TransformedField {
  public:
    CurvesField(CurveData * curves, BoardEnum board, unsigned int version):
      TransformedField(internalField),
      internalField("Curves"),
      curves(curves),
      board(board),
      version(version),
      maxCurves(MAX_CURVES(board, version)),
      maxPoints(IS_ARM(board) ? O9X_ARM_NUM_POINTS : O9X_NUM_POINTS)
    {
      for (int i=0; i<maxCurves; i++) {
        if (IS_TARANIS(board) && version >= 216) {
          internalField.Append(new UnsignedField<3>((unsigned int &)curves[i].type));
          internalField.Append(new BoolField<1>(curves[i].smooth));
          internalField.Append(new SpareBitsField<4>());
          internalField.Append(new ConversionField< SignedField<8> >(curves[i].count, -5));
        }
        else if (IS_ARM(board)) {
          internalField.Append(new SignedField<16>(_curves[i]));
        }
        else {
          internalField.Append(new SignedField<8>(_curves[i]));
        }
      }

      for (int i=0; i<maxPoints; i++) {
        internalField.Append(new SignedField<8>(_points[i]));
      }
    }

    virtual void beforeExport()
    {
      memset(_points, 0, sizeof(_points));

      int * cur = &_points[0];
      int offset = 0;

      for (int i=0; i<maxCurves; i++) {
        CurveData *curve = &curves[i];
        int size = (curve->type == CurveData::CURVE_TYPE_CUSTOM ? curve->count * 2 - 2 : curve->count);
        if (offset+size > maxPoints) {
          EEPROMWarnings += ::QObject::tr("openTx only accepts %1 points in all curves").arg(maxPoints) + "\n";
          break;
        }
        if (!IS_TARANIS(board) || version < 216) {
          _curves[i] = offset - (5*i);
        }
        for (int j=0; j<curve->count; j++) {
          *cur++ = curve->points[j].y;
        }
        if (curve->type == CurveData::CURVE_TYPE_CUSTOM) {
          for (int j=1; j<curve->count-1; j++) {
            *cur++ = curve->points[j].x;
          }
        }
        offset += size;
      }
    }

    virtual void afterImport()
    {
      int * cur = &_points[0];

      for (int i=0; i<maxCurves; i++) {
        CurveData *curve = &curves[i];
        if (!IS_TARANIS(board) || version < 216) {
          int * next = &_points[5*(i+1) + _curves[i]];
          int size = next - cur;
          if (size % 2 == 0) {
            curve->count = (size / 2) + 1;
            curve->type = CurveData::CURVE_TYPE_CUSTOM;
          }
          else {
            curve->count = size;
            curve->type = CurveData::CURVE_TYPE_STANDARD;
          }
        }

        for (int j=0; j<curve->count; j++) {
          curve->points[j].y = *cur++;
        }

        if (curve->type == CurveData::CURVE_TYPE_CUSTOM) {
          curve->points[0].x = -100;
          for (int j=1; j<curve->count-1; j++)
            curve->points[j].x = *cur++;
          curve->points[curve->count-1].x = +100;
        }
        else {
          for (int j=0; j<curve->count; j++)
            curve->points[j].x = -100 + (200*i) / (curve->count-1);
        }
      }
    }

  protected:
    StructField internalField;
    CurveData *curves;
    BoardEnum board;
    unsigned int version;
    int maxCurves;
    int maxPoints;
    int _curves[C9X_MAX_CURVES];
    int _points[C9X_MAX_CURVES*C9X_MAX_POINTS*2];
};

class LogicalSwitchesFunctionsTable: public ConversionTable {

  public:
    LogicalSwitchesFunctionsTable(BoardEnum board, unsigned int version)
    {
      int val=0;
      bool afterrelease21March2013 = IS_AFTER_RELEASE_21_MARCH_2013(board, version);
      addConversion(LS_FN_OFF, val++);
      if (afterrelease21March2013)
        addConversion(LS_FN_VEQUAL, val++);
      addConversion(LS_FN_VPOS, val++);
      addConversion(LS_FN_VNEG, val++);
      if (IS_ARM(board) && version >= 216) val++; // later RANGE
      addConversion(LS_FN_APOS, val++);
      addConversion(LS_FN_ANEG, val++);
      addConversion(LS_FN_AND, val++);
      addConversion(LS_FN_OR, val++);
      addConversion(LS_FN_XOR, val++);
      if (IS_ARM(board) && version >= 216) addConversion(LS_FN_STAY, val++);
      addConversion(LS_FN_EQUAL, val++);
      if (!afterrelease21March2013)
        addConversion(LS_FN_NEQUAL, val++);
      addConversion(LS_FN_GREATER, val++);
      addConversion(LS_FN_LESS, val++);
      if (!afterrelease21March2013) {
        addConversion(LS_FN_EGREATER, val++);
        addConversion(LS_FN_ELESS, val++);
      }
      addConversion(LS_FN_DPOS, val++);
      addConversion(LS_FN_DAPOS, val++);
      addConversion(LS_FN_TIMER, val++);
      if (version >= 216)
        addConversion(LS_FN_STICKY, val++);
    }
};

class AndSwitchesConversionTable: public ConversionTable {

  public:
    AndSwitchesConversionTable(BoardEnum board, unsigned int version)
    {
      int val=0;
      addConversion(RawSwitch(SWITCH_TYPE_NONE), val++);
      
      if (IS_TARANIS(board)) {
        for (int i=1; i<=MAX_SWITCHES_POSITION(board); i++) {
          int s = switchIndex(i, board, version);
          addConversion(RawSwitch(SWITCH_TYPE_SWITCH, -s), -val);
          addConversion(RawSwitch(SWITCH_TYPE_SWITCH, s), val++);
        }
        for (int i=1; i<=MAX_CUSTOM_SWITCHES(board, version); i++) {
          addConversion(RawSwitch(SWITCH_TYPE_VIRTUAL, -i), -val);
          addConversion(RawSwitch(SWITCH_TYPE_VIRTUAL, i), val++);
        }
      }
      else if (board == BOARD_SKY9X) {
        for (int i=1; i<=8; i++) {
          int s = switchIndex(i, board, version);
          addConversion(RawSwitch(SWITCH_TYPE_SWITCH, -s), -val);
          addConversion(RawSwitch(SWITCH_TYPE_SWITCH, s), val++);
        }
        for (int i=1; i<=MAX_CUSTOM_SWITCHES(board, version); i++) {
          addConversion(RawSwitch(SWITCH_TYPE_VIRTUAL, -i), -val);
          addConversion(RawSwitch(SWITCH_TYPE_VIRTUAL, i), val++);
        }
      }
      else {
        for (int i=1; i<=9; i++) {
          int s = switchIndex(i, board, version);
          addConversion(RawSwitch(SWITCH_TYPE_SWITCH, s), val++);
        }
        for (int i=1; i<=7; i++) {
          addConversion(RawSwitch(SWITCH_TYPE_VIRTUAL, i), val++);
        }
      }
    }

    static ConversionTable * getInstance(BoardEnum board, unsigned int version)
    {
      if (IS_ARM(board) && version >= 216)
        return new SwitchesConversionTable(board, version);
      else
        return new AndSwitchesConversionTable(board, version);

#if 0
      static std::list<Cache> internalCache;

      for (std::list<Cache>::iterator it=internalCache.begin(); it!=internalCache.end(); it++) {
        Cache element = *it;
        if (element.board == board && element.version == version && element.flags == flags)
          return element.table;
      }

      Cache element(board, version, flags, new SwitchesConversionTable(board, version, flags));
      internalCache.push_back(element);
      return element.table;
#endif
    }


  protected:

    void addConversion(const RawSwitch & sw, const int b)
    {
      ConversionTable::addConversion(sw.toValue(), b);
    }
};

class LogicalSwitchField: public TransformedField {
  public:
    LogicalSwitchField(LogicalSwitchData & csw, BoardEnum board, unsigned int version, unsigned int variant):
      TransformedField(internalField),
      internalField("LogicalSwitch"),
      csw(csw),
      board(board),
      version(version),
      variant(variant),
      functionsConversionTable(board, version),
      sourcesConversionTable(SourcesConversionTable::getInstance(board, version, variant, (version >= 214 || (!IS_ARM(board) && version >= 213)) ? 0 : FLAG_NOSWITCHES)),
      switchesConversionTable(SwitchesConversionTable::getInstance(board, version)),
      andswitchesConversionTable(AndSwitchesConversionTable::getInstance(board, version)),
      v1(0),
      v2(0),
      v3(0)
    {
      if (IS_ARM(board) && version >= 216) {
        internalField.Append(new SignedField<8>(v1));
        internalField.Append(new SignedField<16>(v2));
        internalField.Append(new SignedField<16>(v3));
      }
      else if (IS_ARM(board) && version >= 215) {
        internalField.Append(new SignedField<16>(v1));
        internalField.Append(new SignedField<16>(v2));
      }
      else {
        internalField.Append(new SignedField<8>(v1));
        internalField.Append(new SignedField<8>(v2));
      }

      if (IS_ARM(board)) {
        internalField.Append(new ConversionField< UnsignedField<8> >(csw.func, &functionsConversionTable, "Function"));
        internalField.Append(new UnsignedField<8>(csw.delay));
        internalField.Append(new UnsignedField<8>(csw.duration));
        if (version >= 214) {
          internalField.Append(new ConversionField< SignedField<8> >((int &)csw.andsw, andswitchesConversionTable, "AND switch"));
        }
      }
      else {
        if (version >= 213) {
          internalField.Append(new ConversionField< UnsignedField<4> >(csw.func, &functionsConversionTable, "Function"));
          internalField.Append(new ConversionField< UnsignedField<4> >((unsigned int &)csw.andsw, andswitchesConversionTable, "AND switch"));
        }
        else {
          internalField.Append(new ConversionField< UnsignedField<8> >(csw.func, &functionsConversionTable, "Function"));
        }
      }
    }

    virtual void beforeExport()
    {
      if (csw.func == LS_FN_TIMER) {
        v1 = csw.val1;
        v2 = csw.val2;
      }
      else if (csw.func == LS_FN_STAY) {
        switchesConversionTable->exportValue(csw.val1, v1);
        v2 = csw.val2;
        v3 = csw.val3;
      }
      else if ((csw.func >= LS_FN_AND && csw.func <= LS_FN_XOR) || csw.func == LS_FN_STICKY) {
        switchesConversionTable->exportValue(csw.val1, v1);
        switchesConversionTable->exportValue(csw.val2, v2);
      }
      else if (csw.func >= LS_FN_EQUAL && csw.func <= LS_FN_ELESS) {
        sourcesConversionTable->exportValue(csw.val1, v1);
        sourcesConversionTable->exportValue(csw.val2, v2);
      }
      else if (csw.func != LS_FN_OFF) {
        sourcesConversionTable->exportValue(csw.val1, v1);
        v2 = csw.val2;
      }
    }

    virtual void afterImport()
    {
      if (csw.func == LS_FN_TIMER) {
        csw.val1 = v1;
        csw.val2 = v2;
      }
      else if (csw.func == LS_FN_STAY) {
        switchesConversionTable->importValue(v1, csw.val1);
        csw.val2 = v2;
        csw.val3 = v3;
      }
      else if ((csw.func >= LS_FN_AND && csw.func <= LS_FN_XOR) || csw.func == LS_FN_STICKY) {
        switchesConversionTable->importValue(v1, csw.val1);
        switchesConversionTable->importValue(v2, csw.val2);
      }
      else if (csw.func >= LS_FN_EQUAL && csw.func <= LS_FN_ELESS) {
        sourcesConversionTable->importValue((uint8_t)v1, csw.val1);
        sourcesConversionTable->importValue((uint8_t)v2, csw.val2);
      }
      else if (csw.func != LS_FN_OFF) {
        sourcesConversionTable->importValue((uint8_t)v1, csw.val1);
        csw.val2 = v2;
      }
    }

  protected:
    StructField internalField;
    LogicalSwitchData & csw;
    BoardEnum board;
    unsigned int version;
    unsigned int variant;
    LogicalSwitchesFunctionsTable functionsConversionTable;
    SourcesConversionTable * sourcesConversionTable;
    SwitchesConversionTable * switchesConversionTable;
    ConversionTable * andswitchesConversionTable;
    int v1;
    int v2;
    int v3;
};

class CustomFunctionsConversionTable: public ConversionTable {

  public:
    CustomFunctionsConversionTable(BoardEnum board, unsigned int version)
    {
      int val=0;

      if (version >= 216) {
        for (int i=0; i<32; i++) {
          addConversion(i, val);
        }
        val++;
      }
      else if (IS_ARM(board) || version < 213) {
        for (int i=0; i<16; i++) {
          addConversion(val, val);
          val++;
        }
      }
      else {
        for (int i=0; i<16; i++) {
          addConversion(i, i / 4);
        }
        val+=4;
      }

      if (version >= 216) {
        addConversion(FuncTrainer, val);
        addConversion(FuncTrainerRUD, val);
        addConversion(FuncTrainerELE, val);
        addConversion(FuncTrainerTHR, val);
        addConversion(FuncTrainerAIL, val);
        val++;
      }
      else {
        addConversion(FuncTrainer, val++);
        addConversion(FuncTrainerRUD, val++);
        addConversion(FuncTrainerELE, val++);
        addConversion(FuncTrainerTHR, val++);
        addConversion(FuncTrainerAIL, val++);
      }
      addConversion(FuncInstantTrim, val++);
      if (version >= 216) {
        addConversion(FuncReset, val++);
        if (IS_ARM(board)) {
          addConversion(FuncSetTimer1, val);
          addConversion(FuncSetTimer2, val);
          val++;
        }
        for (int i=0; i<MAX_GVARS(board, version); i++)
          addConversion(FuncAdjustGV1+i, val);
        val++;
        if (IS_ARM(board))
          addConversion(FuncVolume, val++);
        addConversion(FuncPlaySound, val++);
        addConversion(FuncPlayPrompt, val++);
        if (version >= 213 && !IS_ARM(board))
          addConversion(FuncPlayBoth, val++);
        addConversion(FuncPlayValue, val++);
        if (IS_ARM(board)) {
          addConversion(FuncBackgroundMusic, val++);
          addConversion(FuncBackgroundMusicPause, val++);
        }
        addConversion(FuncVario, val++);
        addConversion(FuncPlayHaptic, val++);
        if (board == BOARD_GRUVIN9X || IS_ARM(board) )
          addConversion(FuncLogs, val++);
        addConversion(FuncBacklight, val++);
      }
      else {
        addConversion(FuncPlaySound, val++);
        if (!IS_TARANIS(board))
          addConversion(FuncPlayHaptic, val++);
        addConversion(FuncReset, val++);
        addConversion(FuncVario, val++);
        addConversion(FuncPlayPrompt, val++);
        if (version >= 213 && !IS_ARM(board))
          addConversion(FuncPlayBoth, val++);
        addConversion(FuncPlayValue, val++);
        if (board == BOARD_GRUVIN9X || IS_ARM(board) )
          addConversion(FuncLogs, val++);
        if (IS_ARM(board))
          addConversion(FuncVolume, val++);
        addConversion(FuncBacklight, val++);
        if (IS_ARM(board)) {
          addConversion(FuncBackgroundMusic, val++);
          addConversion(FuncBackgroundMusicPause, val++);
        }
        for (int i=0; i<5; i++)
          addConversion(FuncAdjustGV1+i, val++);
      }
    }
};

template <int N>
class SwitchesWarningField: public TransformedField {
  public:
    SwitchesWarningField(unsigned int & sw, BoardEnum board, unsigned int version):
      TransformedField(internalField),
      internalField(_sw, "SwitchesWarning"),
      sw(sw),
      board(board),
      version(version)
    {
    }

    virtual void beforeExport()
    {
        _sw = sw;
    }

    virtual void afterImport()
    {
      bool afterrelease21March2013 = IS_AFTER_RELEASE_21_MARCH_2013(board, version);
      if (afterrelease21March2013 && version < 216) {
        sw = _sw >> 1;
      }
      else if (!afterrelease21March2013) {
        sw = ((_sw & 0x30) >> 4) + ((_sw & 0x0E) << 1) + ((_sw & 0xC0) >> 1);
      }
      else {
        sw = _sw;
      }
    }

  protected:
    UnsignedField<N> internalField;
    unsigned int &sw;
    unsigned int _sw;
    BoardEnum board;
    unsigned int version;
};

class ArmCustomFunctionField: public TransformedField {
  public:
    ArmCustomFunctionField(FuncSwData & fn, BoardEnum board, unsigned int version, unsigned int variant):
      TransformedField(internalField),
      internalField("CustomFunction"),
      fn(fn),
      board(board),
      version(version),
      variant(variant),
      functionsConversionTable(board, version),
      sourcesConversionTable(SourcesConversionTable::getInstance(board, version, variant, version >= 216 ? 0 : FLAG_NONONE)),
      _active(0)
    {
      memset(_param, 0, sizeof(_param));

      internalField.Append(new SwitchField<8>(fn.swtch, board, version));
      internalField.Append(new ConversionField< UnsignedField<8> >((unsigned int &)fn.func, &functionsConversionTable, "Function", ::QObject::tr("OpenTX on this board doesn't accept this function")));

      if (IS_TARANIS(board))
        internalField.Append(new CharField<10>(_param, false));
      else
        internalField.Append(new CharField<6>(_param, false));

      if (version >= 216) {
        internalField.Append(new UnsignedField<8>(_active));
      }
      else if (version >= 214) {
        internalField.Append(new UnsignedField<2>(_mode));
        internalField.Append(new UnsignedField<6>(_active));
      }
      else {
        internalField.Append(new UnsignedField<8>((unsigned int &)_active));
        internalField.Append(new SpareBitsField<8>());
      }
    }

    virtual void beforeExport()
    {
      _mode = 0;

      if (fn.func == FuncPlaySound || fn.func == FuncPlayPrompt || fn.func == FuncPlayValue)
        _active = (version >= 216 ? fn.repeatParam : (fn.repeatParam/5));
      else
        _active = (fn.enabled ? 1 : 0);

      if (fn.func >= FuncSafetyCh1 && fn.func <= FuncSafetyCh32) {
        if (version >= 216) {
          *((uint16_t *)_param) = fn.param;
          *((uint8_t *)(_param+3)) = fn.func - FuncSafetyCh1;
        }
        else {
          *((uint32_t *)_param) = fn.param;
        }
      }
      else if (fn.func >= FuncTrainer && fn.func <= FuncTrainerAIL) {
        if (version >= 216)
          *((uint8_t *)(_param+3)) = fn.func - FuncTrainer;
      }
      else if (fn.func >= FuncSetTimer1 && fn.func <= FuncSetTimer2) {
        if (version >= 216) {
          *((uint16_t *)_param) = fn.param;
          *((uint8_t *)(_param+3)) = fn.func - FuncSetTimer1;
        }
      }
      else if (fn.func == FuncPlayPrompt || fn.func == FuncBackgroundMusic) {
        memcpy(_param, fn.paramarm, sizeof(_param));
      }
      else if (fn.func >= FuncAdjustGV1 && fn.func <= FuncAdjustGVLast) {
        if (version >= 216) {
          *((uint8_t *)(_param+2)) = fn.adjustMode;
          *((uint8_t *)(_param+3)) = fn.func - FuncAdjustGV1;
          unsigned int value;
          if (fn.adjustMode == 1)
            sourcesConversionTable->exportValue(fn.param, (int &)value);
          else if (fn.adjustMode == 2)
            value = RawSource(fn.param).index;
          else
            value = fn.param;
          *((uint16_t *)_param) = value;
        }
        else if (version >= 214) {
          unsigned int value;
          _mode = fn.adjustMode;
          if (fn.adjustMode == 1)
            sourcesConversionTable->exportValue(fn.param, (int &)value);
          else if (fn.adjustMode == 2)
            value = RawSource(fn.param).index;
          else
            value = fn.param;
          *((uint32_t *)_param) = value;
        }
        else {
          unsigned int value;
          sourcesConversionTable->exportValue(fn.param, (int &)value);
          *((uint32_t *)_param) = value;
        }
      }
      else if (fn.func == FuncPlayValue || fn.func == FuncVolume) {
        unsigned int value;
        sourcesConversionTable->exportValue(fn.param, (int &)value);
        if (version >= 216)
          *((uint16_t *)_param) = value;
        else
          *((uint32_t *)_param) = value;
      }
      else {
        *((uint32_t *)_param) = fn.param;
      }
    }

    virtual void afterImport()
    {
      if (fn.func == FuncPlaySound || fn.func == FuncPlayPrompt || fn.func == FuncPlayValue)
        fn.repeatParam = (version >= 216 ? _active : (_active*5));
      else
        fn.enabled = (_active & 0x01);

      unsigned int value=0, mode=0, index=0;
      if (version >= 216) {
        value = *((uint16_t *)_param);
        mode = *((uint8_t *)(_param+2));
        index = *((uint8_t *)(_param+3));
      }
      else {
        value = *((uint32_t *)_param);
      }

      if (fn.func >= FuncSafetyCh1 && fn.func <= FuncSafetyCh32) {
        fn.func = AssignFunc(fn.func + index);
        fn.param = (int)value;
      }
      else if (fn.func >= FuncSetTimer1 && fn.func <= FuncSetTimer2) {
        fn.func = AssignFunc(fn.func + index);
        fn.param = (int)value;
      }
      else if (fn.func >= FuncTrainer && fn.func <= FuncTrainerAIL) {
        fn.func = AssignFunc(fn.func + index);
        fn.param = value;
      }
      else if (fn.func == FuncPlayPrompt || fn.func == FuncBackgroundMusic) {
        memcpy(fn.paramarm, _param, sizeof(fn.paramarm));
      }
      else if (fn.func == FuncVolume) {
        sourcesConversionTable->importValue(value, (int &)fn.param);
      }
      else if (fn.func >= FuncAdjustGV1 && fn.func <= FuncAdjustGVLast) {
        if (version >= 216) {
          fn.func = AssignFunc(fn.func + index);
          fn.adjustMode = mode;
          if (fn.adjustMode == 1)
            sourcesConversionTable->importValue(value, (int &)fn.param);
          else if (fn.adjustMode == 2)
            fn.param = RawSource(SOURCE_TYPE_GVAR, value).toValue();
          else
            fn.param = value;
        }
        else if (version >= 214) {
          fn.adjustMode = _mode;
          if (fn.adjustMode == 1)
            sourcesConversionTable->importValue(value, (int &)fn.param);
          else if (fn.adjustMode == 2)
            fn.param = RawSource(SOURCE_TYPE_GVAR, value).toValue();
          else
            fn.param = value;
        }
        else {
          sourcesConversionTable->importValue(value, (int &)fn.param);
        }
      }
      else if (fn.func == FuncPlayValue) {
        if (version >= 213)
          sourcesConversionTable->importValue(value, (int &)fn.param);
        else
          SourcesConversionTable::getInstance(board, version, variant, FLAG_NONONE|FLAG_NOSWITCHES)->importValue(value, (int &)fn.param);
      }
      else {
        fn.param = value;
      }
    }

  protected:
    StructField internalField;
    FuncSwData & fn;
    BoardEnum board;
    unsigned int version;
    unsigned int variant;
    CustomFunctionsConversionTable functionsConversionTable;
    SourcesConversionTable * sourcesConversionTable;
    char _param[10];
    unsigned int _active;
    unsigned int _mode;
};

class AvrCustomFunctionField: public TransformedField {
  public:
    AvrCustomFunctionField(FuncSwData & fn, BoardEnum board, unsigned int version, unsigned int variant):
      TransformedField(internalField),
      internalField("CustomFunction"),
      fn(fn),
      board(board),
      version(version),
      variant(variant),
      functionsConversionTable(board, version),
      sourcesConversionTable(SourcesConversionTable::getInstance(board, version, variant, version >= 216 ? 0 : FLAG_NONONE)),
      _param(0),
      _union_param(0),
      _active(0)
    {
      if (version >= 216) {
        internalField.Append(new SwitchField<6>(fn.swtch, board, version));
        internalField.Append(new ConversionField< UnsignedField<4> >((unsigned int &)fn.func, &functionsConversionTable, "Function", ::QObject::tr("OpenTX on this board doesn't accept this function")));
        internalField.Append(new UnsignedField<5>(_union_param));
        internalField.Append(new UnsignedField<1>(_active));
      }
      if (version >= 213) {
        internalField.Append(new SwitchField<8>(fn.swtch, board, version));
        internalField.Append(new UnsignedField<3>(_union_param));
        internalField.Append(new ConversionField< UnsignedField<5> >((unsigned int &)fn.func, &functionsConversionTable, "Function", ::QObject::tr("OpenTX on this board doesn't accept this function")));
      }
      else {
        internalField.Append(new SwitchField<8>(fn.swtch, board, version));
        internalField.Append(new ConversionField< UnsignedField<7> >((unsigned int &)fn.func, &functionsConversionTable, "Function", ::QObject::tr("OpenTX on this board doesn't accept this function")));
        internalField.Append(new BoolField<1>((bool &)fn.enabled));
      }
      internalField.Append(new UnsignedField<8>(_param));
    }

    virtual void beforeExport()
    {
      _param = fn.param;
      _active = (fn.enabled ? 1 : 0);

      if (fn.func >= FuncSafetyCh1 && fn.func <= FuncSafetyCh32) {
        if (version >= 216)
          _union_param = fn.func - FuncSafetyCh1;
        else if (version >= 213)
          _active += ((fn.func % 4) << 1);
      }
      else if (fn.func >= FuncTrainer && fn.func <= FuncTrainerAIL) {
        if (version >= 216)
          _union_param = fn.func - FuncTrainer;
      }
      else if (fn.func >= FuncAdjustGV1 && fn.func <= FuncAdjustGVLast) {
        if (version >= 216) {
          _union_param = fn.adjustMode;
          _union_param += (fn.func - FuncAdjustGV1) << 2;
          if (fn.adjustMode == 1)
            sourcesConversionTable->exportValue(fn.param, (int &)_param);
          else if (fn.adjustMode == 2)
            _param = RawSource(fn.param).index;
        }
        else if (version >= 213) {
          _active += (fn.adjustMode << 1);
          if (fn.adjustMode == 1)
            sourcesConversionTable->exportValue(fn.param, (int &)_param);
          else if (fn.adjustMode == 2)
            _param = RawSource(fn.param).index;
        }
        else {
          sourcesConversionTable->exportValue(fn.param, (int &)_param);
        }
      }
      else if (fn.func == FuncPlayValue) {
        if (version >= 216) {
          _union_param = fn.repeatParam / 10;
          sourcesConversionTable->exportValue(fn.param, (int &)_param);
        }
        else if (version >= 213) {
          _active = fn.repeatParam / 10;
          sourcesConversionTable->exportValue(fn.param, (int &)_param);
        }
        else {
          SourcesConversionTable::getInstance(board, version, variant, FLAG_NONONE|FLAG_NOSWITCHES)->exportValue(fn.param, (int &)_param);
        }
      }
      else if (fn.func == FuncPlaySound || fn.func == FuncPlayPrompt || fn.func == FuncPlayBoth) {
        if (version >= 216) {
          _union_param = fn.repeatParam / 10;
        }
        else if (version >= 213) {
          _active = fn.repeatParam / 10;
        }
      }
    }

    virtual void afterImport()
    {
      fn.param = _param;
      if (version >= 213) {
        fn.enabled = (_active & 0x01);
      }

      if (fn.func >= FuncSafetyCh1 && fn.func <= FuncSafetyCh32) {
        if (version >= 216)
          fn.func = AssignFunc(fn.func + _union_param);
        else if (version >= 213)
          fn.func = AssignFunc(((fn.func >> 2) << 2) + ((_active >> 1) & 0x03));
        fn.param = (int8_t)fn.param;
      }
      else if (fn.func >= FuncTrainer && fn.func <= FuncTrainerAIL) {
        if (version >= 216)
          fn.func = AssignFunc(fn.func + _union_param);
      }
      else if (fn.func >= FuncAdjustGV1 && fn.func <= FuncAdjustGVLast) {
        if (version >= 216) {
          fn.func = AssignFunc(fn.func + (_union_param >> 2));
          fn.adjustMode = (_union_param & 0x03);
        }
        else if (version >= 213) {
          fn.adjustMode = ((_active >> 1) & 0x03);
          if (fn.adjustMode == 1)
            sourcesConversionTable->importValue(_param, (int &)fn.param);
          else if (fn.adjustMode == 2)
            fn.param = RawSource(SOURCE_TYPE_GVAR, _param).toValue();
        }
        else {
          sourcesConversionTable->importValue(_param, (int &)fn.param);
        }
      }
      else if (fn.func == FuncPlayValue) {
        if (version >= 216) {
          fn.repeatParam = _union_param * 10;
          sourcesConversionTable->importValue(_param, (int &)fn.param);
        }
        else if (version >= 213) {
          fn.repeatParam = _active * 10;
          sourcesConversionTable->importValue(_param, (int &)fn.param);
        }
        else {
          SourcesConversionTable::getInstance(board, version, variant, FLAG_NONONE|FLAG_NOSWITCHES)->importValue(_param, (int &)fn.param);
        }
      }
      else if (fn.func == FuncPlaySound || fn.func == FuncPlayPrompt || fn.func == FuncPlayBoth) {
        if (version >= 216)
          fn.repeatParam = _union_param * 10;
        else if (version >= 213)
          fn.repeatParam = _active * 10;
      }
    }

  protected:
    StructField internalField;
    FuncSwData & fn;
    BoardEnum board;
    unsigned int version;
    unsigned int variant;
    CustomFunctionsConversionTable functionsConversionTable;
    SourcesConversionTable * sourcesConversionTable;
    unsigned int _param;
    unsigned int _mode;
    unsigned int _union_param;
    unsigned int _active;
};

class FrskyScreenField: public DataField {
  public:
    FrskyScreenField(FrSkyScreenData & screen, BoardEnum board, unsigned int version):
      DataField("Frsky Screen"),
      screen(screen),
      board(board),
      version(version)
    {
      for (int i=0; i<4; i++) {
        bars.Append(new UnsignedField<8>(_screen.body.bars[i].source));
        bars.Append(new UnsignedField<8>(_screen.body.bars[i].barMin));
        bars.Append(new UnsignedField<8>(_screen.body.bars[i].barMax));
      }

      int columns=(IS_TARANIS(board) ? 3:2);
      for (int i=0; i<4; i++) {
        for (int j=0; j<columns; j++) {
          numbers.Append(new UnsignedField<8>(_screen.body.lines[i].source[j]));
        }
      }
      if (!IS_TARANIS(board)) {
        for (int i=0; i<4; i++) {
          numbers.Append(new SpareBitsField<8>());
        }
      }
    }

    virtual void ExportBits(QBitArray & output)
    {
      _screen = screen;

      bool afterrelease21March2013 = IS_AFTER_RELEASE_21_MARCH_2013(board, version);
      if (!afterrelease21March2013) {
        for (int i=0; i<4; i++) {
          if (_screen.body.bars[i].source > 0)
            _screen.body.bars[i].source--;
        }
        int columns=(IS_TARANIS(board) ? 3:2);
        for (int i=0; i<4; i++) {
          for (int j=0; j<columns;j++) {
            if (_screen.body.lines[i].source[j] > 0)
              _screen.body.lines[i].source[j]--;
          }
        }
      }

      if (screen.type == 0)
        numbers.ExportBits(output);
      else
        bars.ExportBits(output);
    }

    virtual void ImportBits(QBitArray & input)
    {
      _screen = screen;

      bool afterrelease21March2013 = IS_AFTER_RELEASE_21_MARCH_2013(board, version);

      // NOTA: screen.type should have been imported first!
      if (screen.type == 0) {
        numbers.ImportBits(input);
        if (!afterrelease21March2013) {
          int columns=(IS_TARANIS(board) ? 3:2);
          for (int i=0; i<4; i++) {
            for (int j=0; j<columns;j++) {
              if (_screen.body.lines[i].source[j] > 0)
                _screen.body.lines[i].source[j]++;
            }
          }
        }
      }
      else {
        bars.ImportBits(input);
        if (!afterrelease21March2013) {
          for (int i=0; i<4; i++) {
            if (_screen.body.bars[i].source > 0)
              _screen.body.bars[i].source++;
          }
        }
      }

      screen = _screen;
    }

    virtual unsigned int size()
    {
      // NOTA: screen.type should have been imported first!
      if (screen.type == 0)
        return numbers.size();
      else
        return bars.size();
    }

  protected:
    FrSkyScreenData & screen;
    FrSkyScreenData _screen;
    BoardEnum board;
    unsigned int version;
    StructField bars;
    StructField numbers;
};

class RSSIConversionTable: public ConversionTable
{
  public:
    RSSIConversionTable(int index)
    {
      addConversion(0, 2-index);
      addConversion(1, 3-index);
      addConversion(2, index ? 3 : 0);
      addConversion(3, 1-index);
    }

    RSSIConversionTable()
    {
    }
};

class VarioConversionTable: public ConversionTable
{
  public:
    VarioConversionTable()
    {
      addConversion(2, 0); // Vario
      addConversion(3, 1); // A1
      addConversion(4, 2); // A2
    }
};

class FrskyField: public StructField {
  public:
    FrskyField(FrSkyData & frsky, BoardEnum board, unsigned int version):
      StructField("FrSky")
    {
      rssiConversionTable[0] = RSSIConversionTable(0);
      rssiConversionTable[1] = RSSIConversionTable(1);

      if (IS_ARM(board)) {
        for (int i=0; i<2; i++) {
          Append(new UnsignedField<8>(frsky.channels[i].ratio, "Ratio"));
          Append(new SignedField<12>(frsky.channels[i].offset, "Offset"));
          Append(new UnsignedField<4>(frsky.channels[i].type, "Type"));
          for (int j=0; j<2; j++)
            Append(new UnsignedField<8>(frsky.channels[i].alarms[j].value, "Alarm value"));
          for (int j=0; j<2; j++)
            Append(new UnsignedField<2>(frsky.channels[i].alarms[j].level));
          for (int j=0; j<2; j++)
            Append(new UnsignedField<1>(frsky.channels[i].alarms[j].greater));
          Append(new SpareBitsField<2>());
          Append(new UnsignedField<8>(frsky.channels[i].multiplier, 0, 5, "Multiplier"));
        }
        Append(new UnsignedField<8>(frsky.usrProto));
        if (version >= 216) {
          Append(new UnsignedField<7>(frsky.voltsSource));
          Append(new BoolField<1>(frsky.altitudeDisplayed));
        }
        else {
          Append(new UnsignedField<8>(frsky.voltsSource));
        }
        Append(new UnsignedField<8>(frsky.blades));
        Append(new UnsignedField<8>(frsky.currentSource));

        Append(new UnsignedField<1>(frsky.screens[0].type));
        Append(new UnsignedField<1>(frsky.screens[1].type));
        Append(new UnsignedField<1>(frsky.screens[2].type));
        Append(new SpareBitsField<5>());

        for (int i=0; i<3; i++) {
          Append(new FrskyScreenField(frsky.screens[i], board, version));
        }
        if (IS_TARANIS(board))
          Append(new ConversionField< UnsignedField<8> >(frsky.varioSource, &varioConversionTable, "Vario Source"));
        else
          Append(new UnsignedField<8>(frsky.varioSource));
        Append(new SignedField<8>(frsky.varioCenterMax));
        Append(new SignedField<8>(frsky.varioCenterMin));
        Append(new SignedField<8>(frsky.varioMin));
        Append(new SignedField<8>(frsky.varioMax));
        for (int i=0; i<2; i++) {
          Append(new ConversionField< UnsignedField<2> >(frsky.rssiAlarms[i].level, &rssiConversionTable[i], "RSSI"));
          Append(new ConversionField< SignedField<6> >(frsky.rssiAlarms[i].value, -45+i*3));
        }
        if (version >= 216) {
          Append(new BoolField<1>(frsky.mAhPersistent));
          Append(new UnsignedField<15>(frsky.storedMah));
          Append(new SignedField<8>(frsky.fasOffset));
        }
      }
      else {
        for (int i=0; i<2; i++) {
          Append(new UnsignedField<8>(frsky.channels[i].ratio, "Ratio"));
          Append(new SignedField<12>(frsky.channels[i].offset, "Offset"));
          Append(new UnsignedField<4>(frsky.channels[i].type, "Type"));
          for (int j=0; j<2; j++)
            Append(new UnsignedField<8>(frsky.channels[i].alarms[j].value, "Alarm value"));
          for (int j=0; j<2; j++)
            Append(new UnsignedField<2>(frsky.channels[i].alarms[j].level));
          for (int j=0; j<2; j++)
            Append(new UnsignedField<1>(frsky.channels[i].alarms[j].greater));
          Append(new UnsignedField<2>(frsky.channels[i].multiplier, 0, 3, "Multiplier"));
        }
        Append(new UnsignedField<2>(frsky.usrProto));
        Append(new UnsignedField<2>(frsky.blades));
        Append(new UnsignedField<1>(frsky.screens[0].type));
        Append(new UnsignedField<1>(frsky.screens[1].type));
        Append(new UnsignedField<2>(frsky.voltsSource));
        Append(new SignedField<4>(frsky.varioMin));
        Append(new SignedField<4>(frsky.varioMax));
        for (int i=0; i<2; i++) {
          Append(new ConversionField< UnsignedField<2> >(frsky.rssiAlarms[i].level, &rssiConversionTable[i], "RSSI level"));
          Append(new ConversionField< SignedField<6> >(frsky.rssiAlarms[i].value, -45+i*3, 0, 0, 100, "RSSI value"));
        }
        for (int i=0; i<2; i++) {
          Append(new FrskyScreenField(frsky.screens[i], board, version));
        }
        Append(new UnsignedField<3>(frsky.varioSource));
        Append(new SignedField<5>(frsky.varioCenterMin));
        Append(new UnsignedField<3>(frsky.currentSource));
        Append(new SignedField<8>(frsky.varioCenterMax));
        if (version >= 216) {
          // TODO int8_t   fasOffset;
          Append(new SpareBitsField<8>());
        }
      }
    }

  protected:
    RSSIConversionTable rssiConversionTable[2];
    VarioConversionTable varioConversionTable;
};

class MavlinkField: public StructField {
  public:
    MavlinkField(MavlinkData & mavlink, BoardEnum board, unsigned int version):
      StructField("MavLink")
    {
      Append(new UnsignedField<4>(mavlink.rc_rssi_scale, "Rc_rssi_scale"));
      Append(new UnsignedField<1>(mavlink.pc_rssi_en, "Pc_rssi_en"));
      Append(new SpareBitsField<3>());
      Append(new SpareBitsField<8>());
      Append(new SpareBitsField<8>());
      Append(new SpareBitsField<8>());
    }
};

int exportPpmDelay(int delay) { return (delay - 300) / 50; }
int importPpmDelay(int delay) { return 300 + 50 * delay; }

Open9xModelDataNew::Open9xModelDataNew(ModelData & modelData, BoardEnum board, unsigned int version, unsigned int variant):
  TransformedField(internalField),
  internalField("ModelData"),
  modelData(modelData),
  board(board),
  variant(variant),
  protocolsConversionTable(board)
{
  sprintf(name, "Model %s", modelData.name);

  if (HAS_LARGE_LCD(board))
    internalField.Append(new ZCharField<12>(modelData.name));
  else
    internalField.Append(new ZCharField<10>(modelData.name));

  bool afterrelease21March2013 = IS_AFTER_RELEASE_21_MARCH_2013(board, version);

  if (afterrelease21March2013)
    internalField.Append(new UnsignedField<8>(modelData.modelId));

  if (IS_TARANIS(board) && version >= 215) {
    internalField.Append(new CharField<10>(modelData.bitmap));
  }

  for (int i=0; i<O9X_MAX_TIMERS; i++) {
    internalField.Append(new SwitchField<8>(modelData.timers[i].mode, board, version, POPULATE_TIMER_MODES));
    if ((IS_ARM(board) || IS_2560(board)) && version >= 216) {
      internalField.Append(new UnsignedField<16>(modelData.timers[i].val));
      internalField.Append(new UnsignedField<2>(modelData.timers[i].countdownBeep));
      internalField.Append(new BoolField<1>(modelData.timers[i].minuteBeep));
      internalField.Append(new BoolField<1>(modelData.timers[i].persistent));
      internalField.Append(new SpareBitsField<4>());
      internalField.Append(new SignedField<16>(modelData.timers[i].pvalue));
    }
    else if (afterrelease21March2013) {
      internalField.Append(new UnsignedField<12>(modelData.timers[i].val));
      internalField.Append(new BoolField<1>((bool &)modelData.timers[i].countdownBeep));
      internalField.Append(new BoolField<1>(modelData.timers[i].minuteBeep));
      if (HAS_PERSISTENT_TIMERS(board)) {
        internalField.Append(new BoolField<1>(modelData.timers[i].persistent));
        internalField.Append(new SpareBitsField<1>());
        internalField.Append(new SignedField<16>(modelData.timers[i].pvalue));
      }
      else {
        internalField.Append(new SpareBitsField<2>());
      }
    }
    else {
      internalField.Append(new UnsignedField<16>(modelData.timers[i].val));
      if (HAS_PERSISTENT_TIMERS(board)) {
        internalField.Append(new BoolField<1>(modelData.timers[i].persistent));
        internalField.Append(new SpareBitsField<15>());
      }
    }
  }

  if (IS_TARANIS(board))
    internalField.Append(new SpareBitsField<3>());
  else
    internalField.Append(new ConversionField< SignedField<3> >(modelData.moduleData[0].protocol, &protocolsConversionTable, "Protocol", ::QObject::tr("OpenTX doesn't accept this protocol")));

  internalField.Append(new BoolField<1>(modelData.thrTrim));

  if (IS_TARANIS(board) || (IS_ARM(board) && version >= 216))
    internalField.Append(new SpareBitsField<4>());
  else
    internalField.Append(new ConversionField< SignedField<4> >(modelData.moduleData[0].channelsCount, &channelsConversionTable, "Channels number", ::QObject::tr("OpenTX doesn't allow this number of channels")));

  if (version >= 216)
    internalField.Append(new SignedField<3>(modelData.trimInc));
  else
    internalField.Append(new ConversionField< SignedField<3> >(modelData.trimInc, +2));

  internalField.Append(new BoolField<1>(modelData.disableThrottleWarning));

  if (IS_TARANIS(board) || (IS_ARM(board) && version >= 216))
    internalField.Append(new BoolField<1>(modelData.displayText));
  else
    internalField.Append(new BoolField<1>(modelData.moduleData[0].ppmPulsePol));

  internalField.Append(new BoolField<1>(modelData.extendedLimits));
  internalField.Append(new BoolField<1>(modelData.extendedTrims));
  internalField.Append(new BoolField<1>(modelData.throttleReversed));

  if (!IS_ARM(board) || version < 216) {
    internalField.Append(new ConversionField< SignedField<8> >(modelData.moduleData[0].ppmDelay, exportPpmDelay, importPpmDelay));
  }

  if (IS_ARM(board) || board==BOARD_GRUVIN9X)
    internalField.Append(new UnsignedField<16>(modelData.beepANACenter));
  else
    internalField.Append(new UnsignedField<8>(modelData.beepANACenter));

  for (int i=0; i<MAX_MIXERS(board, version); i++)
    internalField.Append(new MixField(modelData.mixData[i], board, version));
  for (int i=0; i<MAX_CHANNELS(board, version); i++)
    internalField.Append(new LimitField(modelData.limitData[i], board, version));
  for (int i=0; i<MAX_EXPOS(board, version); i++)
    internalField.Append(new ExpoField(modelData.expoData[i], board, version));
  internalField.Append(new CurvesField(modelData.curves, board, version));
  for (int i=0; i<MAX_CUSTOM_SWITCHES(board, version); i++)
    internalField.Append(new LogicalSwitchField(modelData.customSw[i], board, version, variant));
  for (int i=0; i<MAX_CUSTOM_FUNCTIONS(board, version); i++) {
    if (IS_ARM(board))
      internalField.Append(new ArmCustomFunctionField(modelData.funcSw[i], board, version, variant));
    else
      internalField.Append(new AvrCustomFunctionField(modelData.funcSw[i], board, version, variant));
  }
  internalField.Append(new HeliField(modelData.swashRingData, board, version, variant));
  for (int i=0; i<MAX_PHASES(board, version); i++)
    internalField.Append(new PhaseField(modelData.phaseData[i], i, board, version));

  if (!IS_ARM(board) || version < 216) {
    internalField.Append(new SignedField<8>(modelData.moduleData[0].ppmFrameLength));
  }

  internalField.Append(new UnsignedField<8>(modelData.thrTraceSrc));

  if (!afterrelease21March2013) {
    internalField.Append(new UnsignedField<8>(modelData.modelId));
  }

  if (IS_TARANIS(board))
    internalField.Append(new SwitchesWarningField<16>(modelData.switchWarningStates, board, version));
  else
    internalField.Append(new SwitchesWarningField<8>(modelData.switchWarningStates, board, version));

  if (version >= 216) {
    internalField.Append(new UnsignedField<8>(modelData.nSwToWarn));
  }

  if ((board == BOARD_STOCK || (board == BOARD_M128 && version >= 215)) && (variant & GVARS_VARIANT)) {
    for (int i=0; i<MAX_GVARS(board, version); i++) {
      // on M64 GVARS are common to all phases, and there is no name
      internalField.Append(new SignedField<16>(modelData.phaseData[0].gvars[i]));
    }
  }

  if (board != BOARD_STOCK && (board != BOARD_M128 || version < 215)) {
    for (int i=0; i<MAX_GVARS(board, version); i++) {
      internalField.Append(new ZCharField<6>(modelData.gvars_names[i]));
      if (version >= 216) {
        internalField.Append(new BoolField<1>(modelData.gvars_popups[i]));
        internalField.Append(new SpareBitsField<7>());
      }
    }
  }

  if ((board != BOARD_STOCK && (board != BOARD_M128 || version < 215)) || (variant & FRSKY_VARIANT)) {
    internalField.Append(new FrskyField(modelData.frsky, board, version));
  }
  else if ((board == BOARD_STOCK || board == BOARD_M128) && (variant & MAVLINK_VARIANT)) {
    internalField.Append(new MavlinkField(modelData.mavlink, board, version));
  }

  if (IS_TARANIS(board) && version < 215) {
    internalField.Append(new CharField<10>(modelData.bitmap));
  }

  int modulesCount = 2;

  if (IS_TARANIS(board)) {
    modulesCount = 3;
    internalField.Append(new ConversionField< SignedField<8> >(modelData.moduleData[1].protocol, &protocolsConversionTable, "Protocol", ::QObject::tr("OpenTX doesn't accept this protocol")));
    internalField.Append(new UnsignedField<8>(modelData.trainerMode));
  }

  if (IS_ARM(board) && version >= 215) {
    for (int module=0; module<modulesCount; module++) {
      internalField.Append(new SignedField<8>(subprotocols[module]));
      internalField.Append(new UnsignedField<8>(modelData.moduleData[module].channelsStart));
      internalField.Append(new ConversionField< SignedField<8> >(modelData.moduleData[module].channelsCount, -8));
      internalField.Append(new UnsignedField<8>(modelData.moduleData[module].failsafeMode));
      for (int i=0; i<32; i++)
        internalField.Append(new SignedField<16>(modelData.moduleData[module].failsafeChannels[i]));
      internalField.Append(new ConversionField< SignedField<8> >(modelData.moduleData[module].ppmDelay, exportPpmDelay, importPpmDelay));
      internalField.Append(new SignedField<8>(modelData.moduleData[module].ppmFrameLength));
      internalField.Append(new BoolField<8>(modelData.moduleData[module].ppmPulsePol));
    }
  }

  if (board==BOARD_SKY9X && version >= 216) {
    internalField.Append(new UnsignedField<8>(modelData.nPotsToWarn));
    for (int i=0; i < GetEepromInterface()->getCapability(Pots); i++) {
      internalField.Append(new SignedField<8>(modelData.potPosition[i]));
    }
  }

  if (IS_TARANIS(board)) {
    for (int i=0; i<MAX_CURVES(board, version); i++) {
      internalField.Append(new ZCharField<6>(modelData.curves[i].name));
    }
  }

  if (IS_TARANIS(board) && version >= 216) {
    for (int i=0; i<3; i++) {
      ScriptData & script = modelData.scriptData[i];
      internalField.Append(new ZCharField<10>(script.filename));
      internalField.Append(new ZCharField<10>(script.name));
      for (int j=0; j<10; j++) {
        internalField.Append(new SignedField<8>(script.inputs[j]));
      }
    }
    for (int i=0; i<32; i++) {
      internalField.Append(new ZCharField<4>(modelData.inputNames[i]));
    }
    internalField.Append(new UnsignedField<8>(modelData.nPotsToWarn));
    for (int i=0; i < GetEepromInterface()->getCapability(Pots); i++) {
      internalField.Append(new SignedField<8>(modelData.potPosition[i]));
    }    
  }
}

void Open9xModelDataNew::beforeExport()
{
  // qDebug() << QString("before export model") << modelData.name;

  for (int module=0; module<3; module++) {
    if (modelData.moduleData[module].protocol >= PXX_XJT_X16 && modelData.moduleData[module].protocol <= PXX_XJT_LR12)
      subprotocols[module] = modelData.moduleData[module].protocol - PXX_XJT_X16;
    else if (modelData.moduleData[module].protocol >= LP45 && modelData.moduleData[module].protocol <= DSMX)
      subprotocols[module] = modelData.moduleData[module].protocol - LP45;
    else
      subprotocols[module] = (module==0 ? -1 : 0);
  }
}

void Open9xModelDataNew::afterImport()
{
  // qDebug() << QString("after import model") << modelData.name ;

  for (int module=0; module<3; module++) {
    if (modelData.moduleData[module].protocol == PXX_XJT_X16 || modelData.moduleData[module].protocol == LP45) {
      if (subprotocols[module] >= 0)
        modelData.moduleData[module].protocol += subprotocols[module];
      else
        modelData.moduleData[module].protocol = OFF;
    }
  }
}

Open9xGeneralDataNew::Open9xGeneralDataNew(GeneralSettings & generalData, BoardEnum board, unsigned int version, unsigned int variant):
  TransformedField(internalField),
  internalField("General Settings"),
  generalData(generalData),
  board(board),
  version(version),
  inputsCount(IS_TARANIS(board) ? 8 : 7)
{
  generalData.version = version;
  generalData.variant = variant;

  internalField.Append(new UnsignedField<8>(generalData.version));
  if (version >= 213 || (!IS_ARM(board) && version >= 212))
    internalField.Append(new UnsignedField<16>(generalData.variant));

  if (version >= 216) {
    for (int i=0; i<inputsCount; i++) {
      internalField.Append(new SignedField<16>(generalData.calibMid[i]));
      internalField.Append(new SignedField<16>(generalData.calibSpanNeg[i]));
      internalField.Append(new SignedField<16>(generalData.calibSpanPos[i]));
    }
  }
  else {
    for (int i=0; i<inputsCount; i++)
      internalField.Append(new SignedField<16>(generalData.calibMid[i]));
    for (int i=0; i<inputsCount; i++)
      internalField.Append(new SignedField<16>(generalData.calibSpanNeg[i]));
    for (int i=0; i<inputsCount; i++)
      internalField.Append(new SignedField<16>(generalData.calibSpanPos[i]));
  }

  internalField.Append(new UnsignedField<16>(chkSum));
  internalField.Append(new UnsignedField<8>(generalData.currModel));
  internalField.Append(new UnsignedField<8>(generalData.contrast));
  internalField.Append(new UnsignedField<8>(generalData.vBatWarn));
  internalField.Append(new SignedField<8>(generalData.vBatCalib));
  internalField.Append(new SignedField<8>(generalData.backlightMode));

  for (int i=0; i<NUM_STICKS; i++)
    internalField.Append(new SignedField<16>(generalData.trainer.calib[i]));
  for (int i=0; i<NUM_STICKS; i++) {
    internalField.Append(new UnsignedField<6>(generalData.trainer.mix[i].src));
    internalField.Append(new UnsignedField<2>(generalData.trainer.mix[i].mode));
    internalField.Append(new SignedField<8>(generalData.trainer.mix[i].weight));
  }

  internalField.Append(new UnsignedField<8>(generalData.view, 0, MAX_VIEWS(board)-1));

  internalField.Append(new SpareBitsField<2>());
  internalField.Append(new BoolField<1>(generalData.fai));
  internalField.Append(new SignedField<2>((int &)generalData.beeperMode));
  internalField.Append(new BoolField<1>(generalData.flashBeep));
  internalField.Append(new BoolField<1>(generalData.disableMemoryWarning));
  internalField.Append(new BoolField<1>(generalData.disableAlarmWarning));

  internalField.Append(new UnsignedField<2>(generalData.stickMode));
  internalField.Append(new SignedField<5>(generalData.timezone));
  internalField.Append(new SpareBitsField<1>());

  internalField.Append(new UnsignedField<8>(generalData.inactivityTimer));
  if (IS_9X(board) && version >= 215) {
    internalField.Append(new UnsignedField<3>(generalData.mavbaud));
  }
  else {
    internalField.Append(new SpareBitsField<1>());
    internalField.Append(new BoolField<1>(generalData.minuteBeep));
    internalField.Append(new BoolField<1>(generalData.preBeep));
  }
  if (version >= 213 || (!IS_ARM(board) && version >= 212))
    internalField.Append(new UnsignedField<3>(generalData.splashMode)); // TODO
  else
    internalField.Append(new SpareBitsField<3>());
  internalField.Append(new SignedField<2>((int &)generalData.hapticMode));

  internalField.Append(new SpareBitsField<8>());
  internalField.Append(new UnsignedField<8>(generalData.backlightDelay));
  internalField.Append(new UnsignedField<8>(generalData.templateSetup));
  internalField.Append(new SignedField<8>(generalData.PPM_Multiplier));
  internalField.Append(new SignedField<8>(generalData.hapticLength));
  internalField.Append(new UnsignedField<8>(generalData.reNavigation));

  internalField.Append(new SignedField<3>(generalData.beeperLength));
  internalField.Append(new UnsignedField<3>(generalData.hapticStrength));
  internalField.Append(new UnsignedField<1>(generalData.gpsFormat));
  internalField.Append(new SpareBitsField<1>()); // unexpectedShutdown

  internalField.Append(new UnsignedField<8>(generalData.speakerPitch));

  if (IS_ARM(board))
    internalField.Append(new ConversionField< SignedField<8> >(generalData.speakerVolume, -12, 0, 0, 23, "Volume"));
  else
    internalField.Append(new ConversionField< SignedField<8> >(generalData.speakerVolume, -7, 0, 0, 7, "Volume"));

  if (version >= 214 || (!IS_ARM(board) && version >= 213)) {
    internalField.Append(new SignedField<8>(generalData.vBatMin));
    internalField.Append(new SignedField<8>(generalData.vBatMax));
  }

  if (IS_ARM(board)) {
    internalField.Append(new UnsignedField<8>(generalData.backlightBright));
    internalField.Append(new SignedField<8>(generalData.currentCalib));
    if (version >= 213) {
      internalField.Append(new SignedField<8>(generalData.temperatureWarn)); // TODO
      internalField.Append(new UnsignedField<8>(generalData.mAhWarn));
      internalField.Append(new SpareBitsField<16>()); // mAhUsed
      internalField.Append(new SpareBitsField<32>()); // globalTimer
      internalField.Append(new SignedField<8>(generalData.temperatureCalib)); // TODO
      internalField.Append(new UnsignedField<8>(generalData.btBaudrate)); // TODO
      internalField.Append(new BoolField<8>(generalData.optrexDisplay)); //TODO
      internalField.Append(new UnsignedField<8>(generalData.sticksGain)); // TODO
    }
    if (version >= 214) {
      internalField.Append(new UnsignedField<8>(generalData.rotarySteps)); // TODO
      internalField.Append(new UnsignedField<8>(generalData.countryCode));
      internalField.Append(new UnsignedField<8>(generalData.imperial));
    }
    if (version >= 215) {
      internalField.Append(new CharField<2>(generalData.ttsLanguage));
      internalField.Append(new SignedField<8>(generalData.beepVolume));
      internalField.Append(new SignedField<8>(generalData.wavVolume));
      internalField.Append(new SignedField<8>(generalData.varioVolume));
      internalField.Append(new SignedField<8>(generalData.backgroundVolume));
    }
    if (IS_TARANIS(board) && version >= 216) {
      internalField.Append(new UnsignedField<8>(generalData.hw_uartMode));
      for (int i=0; i<8; i++) {
        internalField.Append(new UnsignedField<1>(generalData.potsType[i]));
      }
    }
  }
}

void Open9xGeneralDataNew::beforeExport()
{
  uint16_t sum = 0;
  if (version >= 216) {
    int count = 0;
    for (int i=0; i<inputsCount; i++) {
      sum += generalData.calibMid[i];
      if (++count == inputsCount+5) break;
      sum += generalData.calibSpanNeg[i];
      if (++count == inputsCount+5) break;
      sum += generalData.calibSpanPos[i];
      if (++count == inputsCount+5) break;
    }
  }
  else {
    for (int i=0; i<inputsCount; i++)
      sum += generalData.calibMid[i];
    for (int i=0; i<5; i++)
      sum += generalData.calibSpanNeg[i];
  }
  chkSum = sum;
}

void Open9xGeneralDataNew::afterImport()
{
}
