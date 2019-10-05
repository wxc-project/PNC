#pragma once

#define RELEASE_VER 1				 // 0: beta version; 1: release version
#define RELEASE_DATE "July 10 2019"  // Mmm dd yyyy; only used for RELEASE_VER=1

#ifdef __UBOM_ONLY_
#define FILE_VER_MAIN  1 // version number (binary)
#define FILE_VER_MAIN2 0
#define FILE_VER_SUB   0
#define FILE_VER_SUB2  0

#define PRODUCT_VER_MAIN  1 // version number (binary)
#define PRODUCT_VER_MAIN2 0
#define PRODUCT_VER_SUB   0
#define PRODUCT_VER_SUB2 13 
#else
#define FILE_VER_MAIN  1 // version number (binary)
#define FILE_VER_MAIN2 0
#define FILE_VER_SUB   0
#define FILE_VER_SUB2  1

#define PRODUCT_VER_MAIN  1 // version number (binary)
#define PRODUCT_VER_MAIN2 2
#define PRODUCT_VER_SUB   0
#define PRODUCT_VER_SUB2 51 
#endif

// version number (string)
#define ToString2(arg) #arg
#define ToString(arg) ToString2(arg)
#define RELEASE_FILE_VER     ToString(FILE_VER_MAIN) "." ToString(FILE_VER_MAIN2) "." ToString(FILE_VER_SUB) "." ToString(FILE_VER_SUB2)
#define RELEASE_PRODUCT_VER  ToString(PRODUCT_VER_MAIN) "." ToString(PRODUCT_VER_MAIN2) "." ToString(PRODUCT_VER_SUB) "." ToString(PRODUCT_VER_SUB2)


