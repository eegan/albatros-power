typedef unsigned long UINT32;
typedef unsigned short UINT16;

typedef long logDataType;

// Macro to return number of elements in an array
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
