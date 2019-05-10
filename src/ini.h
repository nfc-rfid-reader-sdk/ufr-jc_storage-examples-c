/*
 * ini.h
 */

#ifndef INI_H_
#define INI_H_

#define APP_VERSION				"0.1"

#define EXIT_ON_WRONG_FW_DEPENDENCY
#define MIN_DEPEND_FW_VER_MAJOR		5
#define MIN_DEPEND_FW_VER_MINOR		0
#define MIN_DEPEND_FW_VER_BUILD		19

#define EXIT_ON_WRONG_LIB_DEPENDENCY
#define MIN_DEPEND_LIB_VER_MAJOR	5
#define MIN_DEPEND_LIB_VER_MINOR	0
#define MIN_DEPEND_LIB_VER_BUILD	6

#ifdef __DEBUG
#	if __WIN32 || __WIN64
#		define PORT_NAME	"COM3"
#	else
#		define PORT_NAME	"/dev/ttyS3"
#	endif
#endif

#endif /* INI_H_ */
