#pragma once

#define RELEASE_VER 1				 // 0: beta version; 1: release version
#define RELEASE_DATE "July 10 2019"  // Mmm dd yyyy; only used for RELEASE_VER=1

#define FILE_VER_MAIN  1 // version number (binary)
#define FILE_VER_MAIN2 0
#define FILE_VER_SUB   0
#define FILE_VER_SUB2  1

#define PRODUCT_VER_MAIN  1 // version number (binary)
#define PRODUCT_VER_MAIN2 2
#define PRODUCT_VER_SUB   0
#ifdef _MAPTMA_ZRX
#define PRODUCT_VER_SUB_ZRX 24 
#define PRODUCT_VER_SUB2  PRODUCT_VER_SUB_ZRX
#elif _ARX_2010
#define PRODUCT_VER_SUB_10 59 
#define PRODUCT_VER_SUB2  PRODUCT_VER_SUB_10
#elif _ARX_2007
#define PRODUCT_VER_SUB_07 76 
#define PRODUCT_VER_SUB2  PRODUCT_VER_SUB_07
#else
#define PRODUCT_VER_SUB_05 236 
#define PRODUCT_VER_SUB2  PRODUCT_VER_SUB_05
#endif

// version number (string)
#define ToString2(arg) #arg
#define ToString(arg) ToString2(arg)
#define RELEASE_FILE_VER     ToString(FILE_VER_MAIN) "." ToString(FILE_VER_MAIN2) "." ToString(FILE_VER_SUB) "." ToString(FILE_VER_SUB2)
#define RELEASE_PRODUCT_VER  ToString(PRODUCT_VER_MAIN) "." ToString(PRODUCT_VER_MAIN2) "." ToString(PRODUCT_VER_SUB) "." ToString(PRODUCT_VER_SUB2)




