/////////////////////////////////////////////////////////////////////////////////////
// Program configuration support (EEPROM backed)
// Includes EEPROM support, default values, and accessor function
/////////////////////////////////////////////////////////////////////////////////////

#include "powertrack.h"
#include <EEPROM.h>

/////////////////////////////////////////////////////////////////////////////////////////////////
// EEPROM configuration parameters
/////////////////////////////////////////////////////////////////////////////////////////////////

// Tell it where to store your config data in EEPROM
#define CONFIG_START 0
#define FIELDS_START 32

// ID of the header block
const uint16_t CONFIG_LEVEL = 1;      // increment for each non-backwards compatible configuration structure upgrade
const uint32_t FW_VERSION = 0x10000;  // the current firmware version

/////////////////////////////////////////////////////////////////////////////////////////////////
// EEPROM header data structure
/////////////////////////////////////////////////////////////////////////////////////////////////

bool configNew = false;
const uint32_t MAGIC = 0xABCD1234;

struct valstructtag {
  // validate settings
  struct {
    uint32_t magic;
    uint16_t cfg_level;
  } validation;

  // parameters go here
  uint32_t fw_rev;
  
} cfg = {
  {0, 0} // will be overwritten anyway
  // defaults go here
  , FW_VERSION
};

/////////////////////////////////////////////////////////////////////////////////////////////////
// EEPROM field data structures
// 4 entries must be maintained for each field
/////////////////////////////////////////////////////////////////////////////////////////////////

// Field types
enum e_ft {
  FT_UINT32, FT_INT32, FT_TIME
} ;

// Index values - to be used by set/get accessors
enum e_fndx {
   ndx_vbatOffThresholdMv
  ,ndx_vbatOnThresholdMv
  ,ndx_measureStart 
  ,ndx_measureEnd
  ,ndx_dayStart
  ,ndx_dayEnd
  ,ndx_hoursReserve
  ,ndx_slope 
  ,ndx_secsPerLog
  ,ndx_maxVDataAge
};

// The in-memory copy of the fields, with type tags
struct {
  e_ft type;
  long value;
} eeprom_fields[] = {
   {FT_UINT32, 24500L}  // discharge (load) turn-off, 25 volts
  ,{FT_UINT32, 25750L}  // discharge (load) turn-on, 25.1 volts
  ,{FT_TIME, 23*3600L}  // time to start measurement of discharge curve, 2300h
  ,{FT_TIME, 05*3600L}  // time to end measurement of discharge curve, 0300h
  ,{FT_TIME, 06*3600L}  // start of daytime mode (seconds since midnight)
  ,{FT_TIME, 18*3600L}  // end of daytime mode   (seconds since midnight)
  ,{FT_UINT32, 48L}     // minimum calculated hours of reserve to run during the day
  ,{FT_UINT32, 36360L}  // uV / hour nominal discharge rate (220Ah, 50% discharge, 2V span, 2A load)
  ,{FT_UINT32, 60}      // seconds between log entries
  ,{FT_UINT32, 300}     // maximum seconds between victron data packets (timeout) before load turned off
};

// Field names
char const *fieldNames[] = 
{
    "VBAT_TURNOFF_MV   - low volt cutoff asserted if vbat below this level            "
  , "VBAT_TURNON_MV    - low volt cutoff negated if vbat above this level             "
  , "MEASURE_START     - start vbat measurement for run-during-day decision           "
  , "MEASURE_END       - end voltage measurement for run-during-day decision          "
  , "DAY_START         - turn off load (unless running during day)                    "
  , "DAY_END           - turn on load (if otherwise qualified)                        "
  , "HOURS_RESERVE     - min reserve from MEASURE_START for day run (NEG = no day run)"
  , "SLOPE_UV_PER_HOUR - (not currently used)                                         "
  , "SECS_PER_LOG      - seconds between power log entries                            "
  , "MAX_VIC_DATA_AGE  - seconds of no data to declare victron data timeout           "
};

// TODO: include a field count in the header struct, 
// and initialze fields beyond the current EEPROM field count value, to defaults
// This allows an upgrade which adds fields but preserves previously defined values


/////////////////////////////////////////////////////////////////////////////////////
// cfgInit
// Init (called from main)
/////////////////////////////////////////////////////////////////////////////////////
// Called from main init function
void cfgInit() {
  checkConfig();
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Other external functions
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
// cfgFieldValue
/////////////////////////////////////////////////////////////////////////////////////////////////
// Return field value given an index
long cfg_fieldValue(int ndx)
{
  return eeprom_fields[ndx].value;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// cfgSet (string, string)
// set in-memory value given string versions of index and value
/////////////////////////////////////////////////////////////////////////////////////////////////
// Set in-memory configuration
void cfg_set(const char *indexString, const char *valueString) {
  if (isdigit(*indexString) && isdigit(*valueString)) {
    int index = atoi(indexString);
    long value = parseValue(index, valueString);
    cfg_set(index, value);
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// cfgSet (int, long)
// set in-memory value given index and string
/////////////////////////////////////////////////////////////////////////////////////////////////
// Set in-memory configuration
void cfg_set(int index, long value) {
  BC(eeprom_fields, index);
  eeprom_fields[index].value = value;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// cfgInvalidateEE
// Invalidate EEPROM header, so it loads defaults on the next restart
// Used for testing, or for reinitializing in actual operation
/////////////////////////////////////////////////////////////////////////////////////////////////
void cfg_invalidateEE() {
  EEPROM.write(CONFIG_START+0, 0); // overwrite first MAGIC byte to invalidate EEPROM contents
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// cfgSaveConfig
// save configuration from memory to EEPROM
/////////////////////////////////////////////////////////////////////////////////////////////////
void cfg_saveConfig() {
    uint16_t i;
    for (i=0; i<sizeof cfg; i++)
      EEPROM.write(CONFIG_START + i, *((char*)&cfg + i));
    for (i=0; i<sizeof eeprom_fields; i++)
      EEPROM.write(FIELDS_START + i, *((char*)eeprom_fields + i));
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// cfgDumpFieldValues
// print out all field values with indexes
/////////////////////////////////////////////////////////////////////////////////////////////////
void cfgDumpFieldValues(Stream &p) {
  // p.write("magic:    "); p.println(cfg.validation.magic, 16);
  p.write("Configuration level:   "); p.println(cfg.validation.cfg_level, 10);
  p.write("Newly initialized:     "); p.println(configNew, 10);

  for (uint16_t i=0; i<COUNT_OF(eeprom_fields); i++)
  {
    char buf[100];
    snprintf(buf, sizeof buf, "%d - %s = %s", i, fieldNames[i], fieldValueString(i));
    p.println(buf);
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Internal functions - do not call from outside
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
// checkConfig
/////////////////////////////////////////////////////////////////////////////////////////////////
// Load header block from EEPROM and check if configuration is valid
// If valid, read the rest of the EEPROM
// If invalid, write hard-coded defaults to the EEPROM
void checkConfig() {
  // read out the validation sub-struct
  for (uint16_t i=0; i<sizeof cfg.validation; i++)
    ((byte*)&cfg)[i] = EEPROM.read(CONFIG_START+i);

//  monitorPort.print("Magic number: "); monitorPort.println(cfg.validation.magic);
  
  // check the magic number and the configuration level
  if (  MAGIC  !=  cfg.validation.magic
      || CONFIG_LEVEL <  cfg.validation.cfg_level
  ) {
    // invalid or obsolete configuration - set new flag and fix up validation sub-struct
    monitorPort.println("EEPROM invalid - reinitializing");
    
    configNew = true;
    cfg.validation.magic = MAGIC;
    cfg.validation.cfg_level = CONFIG_LEVEL;

    // save everything (using default values from initializers)
    // TODO: this is lazy compared with just writing what we need
    cfg_saveConfig();
  }
  else {
    // valid configuration - load it
    monitorPort.println("EEPROM valid - loading");
    loadConfig();
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// loadConfig
/////////////////////////////////////////////////////////////////////////////////////////////////
// Load header block from EEPROM and check if configuration is valid
void loadConfig() {
    // load the header block
    for (uint16_t i=sizeof cfg.validation; i<sizeof cfg; i++)
      ((byte*)&cfg)[i] = EEPROM.read(CONFIG_START+i);
     // load the fields array 
    for (uint16_t i=0; i<sizeof eeprom_fields; i++)
      ((byte*) eeprom_fields)[i] = EEPROM.read(FIELDS_START+i);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// fieldValueString - format the field value as a string
/////////////////////////////////////////////////////////////////////////////////////////////////
char *fieldValueString(int ndx)
{
  static char buf[20];
  long v = eeprom_fields[ndx].value;
  switch(eeprom_fields[ndx].type) {
    case FT_UINT32:
    case FT_INT32:
      ltoa(v, buf, 10);
      break;
    case FT_TIME:
      snprintf(buf, sizeof buf, "%02ld:%02ld:%02ld", v/3600L, (v%3600L)/60L, v%60L);
      break;
  }
  return buf;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// parseValue - parse the string value according to the type
/////////////////////////////////////////////////////////////////////////////////////////////////
long parseValue(int ndx, const char *str)
{
  long v;
  int h=0, m=0, s=0;
  switch(eeprom_fields[ndx].type) {
    case FT_UINT32:
    case FT_INT32:
      v = atol(str);
      break;
    case FT_TIME:
      sscanf(str, "%02d:%02d:%02d", &h, &m, &s);
      v = h*3600L+m*60L+s;
      break;
  }
  return v;
}
