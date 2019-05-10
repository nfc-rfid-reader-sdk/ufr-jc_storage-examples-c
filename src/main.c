/*
 ============================================================================
 Project Name: uFR JC Storage Example
 Name        : main.c
 Author      : d-logic (http://www.d-logic.net/nfc-rfid-reader-sdk/)
 Version     : 0.1-alpha
 Copyright   : 2009 - 2019.
 Description : Project in C (Language standard: c99)
 Dependencies: uFR firmware - min. version 5.0.19
               uFRCoder library - min. version 5.0.6
 ============================================================================
 */

/* includes:
 * stdio.h & stdlib.h are included by default (for printf and LARGE_INTEGER.QuadPart (long long) use %lld or %016llx).
 * inttypes.h, stdbool.h & string.h included for various type support and utilities.
 * conio.h is included for windows(dos) console input functions.
 * windows.h is needed for various timer functions (GetTickCount(), QueryPerformanceFrequency(), QueryPerformanceCounter())
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#if __WIN32 || __WIN64
#	include <conio.h>
#	include <windows.h>
#elif linux || __linux__ || __APPLE__
#	define __USE_MISC
#	include <unistd.h>
#	include <termios.h>
#	undef __USE_MISC
#	include "conio_gnu.h"
#else
#	error "Unknown build platform."
#endif
#include <uFCoder.h>
#include "ini.h"
#include "uFR.h"
#include "utils.h"
//------------------------------------------------------------------------------
#define AID			"\xF0" "DLogic" "\x01\x01"
#define AID_LEN		9
//------------------------------------------------------------------------------
void usage(void);
void menu(char key);
UFR_STATUS NewCardInField(uint8_t sak, uint8_t *uid, uint8_t uid_size);
void write(void);
void fastRead(void);
void read(void);
void testCardIsStorageType(void);
//------------------------------------------------------------------------------
int main(void) {
	char key;
	bool card_in_field = false;
	uint8_t old_sak = 0, old_uid_size = 0, old_uid[10];
	uint8_t sak, uid_size, uid[10];
	UFR_STATUS status;

	usage();
	printf(" --------------------------------------------------\n");
	printf("     Please wait while opening uFR NFC reader.\n");
	printf(" --------------------------------------------------\n");

//#ifdef __DEBUG
//	status = ReaderOpenEx(1, PORT_NAME, 1, NULL);
//#else
	status = ReaderOpen();
	s_block_deselect(100);
//#endif
	if (status != UFR_OK) {
		printf("Error while opening device, status is: %s\n", UFR_Status2String(status));
		getchar();
		return EXIT_FAILURE;
	}
//	status = ReaderReset();
	if (status != UFR_OK) {
		ReaderClose();
		printf("Error while opening device, status is: %s\n", UFR_Status2String(status));
		getchar();
		return EXIT_FAILURE;
	}
#if __WIN32 || __WIN64
	Sleep(500);
#else // if linux || __linux__ || __APPLE__
	usleep(500000);
#endif

	if (!CheckDependencies()) {
		ReaderClose();
		getchar();
		return EXIT_FAILURE;
	}

	printf(" --------------------------------------------------\n");
	printf("        uFR NFC reader successfully opened.\n");
	printf(" --------------------------------------------------\n");

#if linux || __linux__ || __APPLE__
	_initTermios(0);
#endif
	do {
		while (!_kbhit()) {
			status = GetCardIdEx(&sak, uid, &uid_size);
			switch (status) {
			case UFR_OK:
				if (card_in_field) {
					if (old_sak != sak || old_uid_size != uid_size
							|| memcmp(old_uid, uid, uid_size)) {
						old_sak = sak;
						old_uid_size = uid_size;
						memcpy(old_uid, uid, uid_size);
						NewCardInField(sak, uid, uid_size);
					}
				} else {
					old_sak = sak;
					old_uid_size = uid_size;
					memcpy(old_uid, uid, uid_size);
					NewCardInField(sak, uid, uid_size);
					card_in_field = true;
				}
				break;
			case UFR_NO_CARD:
				card_in_field = false;
				status = UFR_OK;
				break;
			default:
				ReaderClose();
				printf(
						" Fatal error while trying to read card, status is: %s\n", UFR_Status2String(status));
				getchar();
#if linux || __linux__ || __APPLE__
				_resetTermios();
				tcflush(0, TCIFLUSH); // Clear stdin to prevent characters appearing on prompt
#endif
				return EXIT_FAILURE;
			}
#if __WIN32 || __WIN64
			Sleep(300);
#else // if linux || __linux__ || __APPLE__
			usleep(300000);
#endif
		}

		key = _getch();
		menu(key);
	} while (key != '\x1b');

	ReaderClose();
#if linux || __linux__ || __APPLE__
	_resetTermios();
	tcflush(0, TCIFLUSH); // Clear stdin to prevent characters appearing on prompt
#endif
	return EXIT_SUCCESS;
}
//------------------------------------------------------------------------------
void menu(char key) {

	switch (key) {
	case '1':
		write();
		break;
	case '2':
		fastRead();
		break;
	case '3':
		read();
		break;
	case '4':
		testCardIsStorageType();
		break;
	case '\x1b':
		break;
	default:
		usage();
		break;
	}
}
//------------------------------------------------------------------------------
void usage(void) {
	printf( " +------------------------------------------------+\n"
			" |              DL JC Storage Example             |\n"
			" |                 version "APP_VERSION"                    |\n"
			" +------------------------------------------------+\n"
			"                              For exit, hit escape.\n");
	printf( " --------------------------------------------------\n");
	printf( "  (1) - Write card\n"
			"  (2) - Fast read (streaming method using extended length R-APDUs)\n"
			"  (3) - Read (chunked mechanism, normal speed)\n"
			"  (4) - Is card in field storage type?\n"
			"(Esc) - Quit example\n");
}
//------------------------------------------------------------------------------
UFR_STATUS NewCardInField(uint8_t sak, uint8_t *uid, uint8_t uid_size) {
	UFR_STATUS status;
	uint8_t dl_card_type;

	status = GetDlogicCardType(&dl_card_type);
	if (status != UFR_OK)
		return status;

	printf(" \a-------------------------------------------------------------------\n");
	printf(" Card type: %s, sak = 0x%02X, uid[%d] = ", UFR_DLCardType2String(dl_card_type), sak, uid_size);
	print_hex_ln(uid, uid_size, ":");
	printf(" -------------------------------------------------------------------\n");

	return UFR_OK;
}
//------------------------------------------------------------------------------
void write(void) {
	UFR_STATUS status;
	uint8_t selection_response[16]; // Maximum size of selection response is always 16 bytes

	printf(" -------------------------------------------------------------------\n");
	printf("                        Operation: write                            \n");
	printf(" -------------------------------------------------------------------\n");

	status = SetISO14443_4_DLStorage();
	if (status != UFR_OK) {
		printf(" Error while switching into ISO 14443-4 mode, status is: %s\n", UFR_Status2String(status));
		return;
	}
	do {
		// Select:
		printf(" Sending Select APDU\n");
		status = JCAppSelectByAid(DL_STORAGE_AID, DL_RAW_SIZEOF_SZ(DL_STORAGE_AID), selection_response);
		if (status != UFR_OK) {
			printf(" Error while selecting card application, status is: %s\n", UFR_Status2String(status));
			break;
		}
	} while (0);

	s_block_deselect(100);
}
//------------------------------------------------------------------------------
void fastRead(void) {
	UFR_STATUS status;
	uint8_t selection_response[16]; // Maximum size of selection response is always 16 bytes

	printf(" -------------------------------------------------------------------\n");
	printf("                       Operation: fast read                         \n");
	printf(" -------------------------------------------------------------------\n");

	status = SetISO14443_4_DLStorage();
	if (status != UFR_OK) {
		printf(" Error while switching into ISO 14443-4 mode, status is: %s\n", UFR_Status2String(status));
		return;
	}
	do {
		// Select:
		printf(" Sending Select APDU\n");
		status = JCAppSelectByAid(DL_STORAGE_AID, DL_RAW_SIZEOF_SZ(DL_STORAGE_AID), selection_response);
		if (status != UFR_OK) {
			printf(" Error while selecting card application, status is: %s\n", UFR_Status2String(status));
			break;
		}
	} while (0);

	s_block_deselect(100);
}
//------------------------------------------------------------------------------
void read(void) {
	UFR_STATUS status;
	uint8_t selection_response[16]; // Maximum size of selection response is always 16 bytes

	printf(" -------------------------------------------------------------------\n");
	printf("                         Operation: read                            \n");
	printf(" -------------------------------------------------------------------\n");

	status = SetISO14443_4_DLStorage();
	if (status != UFR_OK) {
		printf(" Error while switching into ISO 14443-4 mode, status is: %s\n", UFR_Status2String(status));
		return;
	}
	do {
		// Select:
		printf(" Sending Select APDU\n");
		status = JCAppSelectByAid(DL_STORAGE_AID, DL_RAW_SIZEOF_SZ(DL_STORAGE_AID), selection_response);
		if (status != UFR_OK) {
			printf(" Error while selecting card application, status is: %s\n", UFR_Status2String(status));
			break;
		}
	} while (0);

	s_block_deselect(100);
}
//------------------------------------------------------------------------------
void testCardIsStorageType(void) {
	UFR_STATUS status;
	uint8_t selection_response[16]; // Maximum size of selection response is always 16 bytes
	uint32_t Ne;
	uint8_t sw[2];
	uint16_t *sw16_ptr = (uint16_t *) &sw;
	LARGE_INTEGER start, end;

	status = SetISO14443_4_DLStorage();
	if (status != UFR_OK) {
		printf(" Error while switching into ISO 14443-4 mode, status is: %s\n", UFR_Status2String(status));
		return;
	}

	printf(" -------------------------------------------------------------------\n");

	do {
		// Select:
		printf(" Sending Select APDU\n");
		Ne = 16;

		QueryPerformanceCounter(&start);
		status = APDUTransceive(0x00, 0xA4, 0x04, 0x00, (const uint8_t *) AID, AID_LEN, selection_response, &Ne, 1, sw);
		if (status != UFR_OK) {
			printf(" Error while selecting card application, status is: %s\n", UFR_Status2String(status));
			break;
		}
		QueryPerformanceCounter(&end);

		printf(" -------------------------------------------------------------------\n");

		if (*sw16_ptr == 0x90) {
			printf(" OK!                Card is DL JC Storage Type                      \n");
		} else {
			printf("           You can't use this card to read and write files          \n");
		}

		printf(" -------------------------------------------------------------------\n");
	} while (0);
	printf("aid size: %d\n", DL_RAW_SIZEOF_SZ(DL_SIGNER_AID));
	printf("Measured time is: %f\n", time_difference_s(start, end));

	s_block_deselect(100);
}
//------------------------------------------------------------------------------
