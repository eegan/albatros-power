typedef unsigned long UINT32;
typedef unsigned short UINT16;

typedef long logDataType;

// Field table

struct fd {
  char *tag;  // the string we expect to see from the Victron
  byte index; // an index into the result array
  byte type;  // Type enumeration  so we know what to expect and how to treat it 
};

// Field type enumerations
const byte FT_int =     1;  // mV, mA, W or 0.01kWh
const byte FT_ON_OFF =  2;  // "OFF" / "ON" in string, --> 0 / 1
const byte FT_bool =    3;  // 0 or 1
const byte FT_string =  4;
const byte FT_checksum = 5; // single byte, could be anything, special treatment

const byte FI_V     = 0;
const byte FI_VPV   = 1;
const byte FI_PPV   = 2;
const byte FI_I     = 3;
const byte FI_IL    = 4;
const byte FI_ILOAD = 5;
const byte FI_Relay = 6;
const byte FI_H19   = 7;
const byte FI_H20   = 8;
const byte FI_H21   = 9;
const byte FI_H22   = 10;
const byte FI_H23   = 11;
const byte FI_ERR   = 12;
const byte FI_CS    = 13;
const byte FI_FW    = 14;
const byte FI_PID   = 15;
const byte FI_SER   = 16;
const byte FI_HSDS  = 17;
const byte FI_MPPT  = 18;
const byte FI_Checksum = 19;

struct fd fieldDescriptors[] = {
   {"V",      FI_V,     FT_int}   // main battery voltage (mV)
  ,{"VPV",    FI_VPV,   FT_int}   // panel voltage (mV)
  ,{"PPV",    FI_PPV,   FT_int}   // panel power (W)
  ,{"I",      FI_I,     FT_int}   // battery current (mA, signed)
  ,{"IL",     FI_IL,    FT_int}   // load current (does not appear on 150/35)
  ,{"LOAD",   FI_ILOAD, FT_ON_OFF} // load state (does not appear on 150/35)
  ,{"Relay",  FI_Relay, FT_ON_OFF}  // relay state (does not appear on 150/35)
  ,{"H19",    FI_H19,   FT_int}   // total yield (resettable) x 0.01kWh
  ,{"H20",    FI_H20,   FT_int}   // yield today x 0.01kWh
  ,{"H21",    FI_H21,   FT_int}   // max power today (W)
  ,{"H22",    FI_H22,   FT_int}   // yield yesterday x 0.01kWh
  ,{"H23",    FI_H23,   FT_int}   // max power yesterday (W)
  ,{"ERR",    FI_ERR,   FT_int} 
  ,{"CS",     FI_CS,    FT_int} 
  ,{"FW",     FI_FW,    FT_string} 
  ,{"PID",    FI_PID,   FT_string} 
  ,{"SER#",   FI_SER,   FT_string} 
  ,{"HSDS",   FI_HSDS,  FT_int} 
  ,{"MPPT",   FI_MPPT,  FT_int}
  ,{"Checksum", FI_Checksum, FT_checksum} // field ID may never be used??
};

// Macro to return number of elements in an array
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
