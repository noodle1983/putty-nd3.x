#ifndef ZMODEM_H
#define ZMODEM_H

/******************************
 *macro
 *****************************/

#define ZPAD '*' 
#define ZDLE 030
#define ZDLEE (ZDLE^0100)
#define ZBIN 'A'
#define ZHEX 'B'
#define ZBIN32 'C'

#define ENQ 005
#define CAN ('X'&037)
#define XOFF ('s'&037)
#define XON ('q'&037)
#define SOH 1
#define STX 2
#define EOT 4

// Frame types
#define ZRQINIT    0
#define ZRINIT     1
#define ZSINIT     2
#define ZACK       3
#define ZFILE      4
#define ZSKIP      5
#define ZNAK       6
#define ZABORT     7
#define ZFIN       8
#define ZRPOS      9
#define ZDATA      10
#define ZEOF       11
#define ZFERR      12
#define ZCRC       13
#define ZCHALLENGE 14
#define ZCOMPL     15
#define ZCAN       16
#define ZFREECNT   17
#define ZCOMMAND   18
#define ZSTDERR    19

/* ZDLE sequences */
#define ZCRCE 'h'	/* CRC next, frame ends, header packet follows */
#define ZCRCG 'i'	/* CRC next, frame continues nonstop */
#define ZCRCQ 'j'	/* CRC next, frame continues, ZACK expected */
#define ZCRCW 'k'	/* CRC next, ZACK expected, end of frame */
#define ZRUB0 'l'	/* Translate to rubout 0177 */
#define ZRUB1 'm'	/* Translate to rubout 0377 */

#define ZF0	3
#define ZF1	2
#define ZF2	1
#define ZF3	0
#define ZP0	0
#define ZP1	1
#define ZP2	2
#define ZP3	3

/* Bit Masks for ZRINIT flags byte ZF0 */
#define CANFDX	0x01	/* Rx can send and receive true FDX */
#define CANOVIO	0x02	/* Rx can receive data during disk I/O */
#define CANBRK	0x04	/* Rx can send a break signal */
#define CANCRY	0x08	/* Receiver can decrypt */
#define CANLZW	0x10	/* Receiver can uncompress */
#define CANFC32	0x20	/* Receiver can use 32 bit Frame Check */
#define ESCCTL  0x40	/* Receiver expects ctl chars to be escaped */
#define ESC8    0x80	/* Receiver expects 8th bit to be escaped */
/* Bit Masks for ZRINIT flags byze ZF1 */
#define ZF1_CANVHDR  0x01  /* Variable headers OK, unused in lrzsz */
#define ZF1_TIMESYNC 0x02 /* nonstandard, Receiver request timesync */

/* Parameters for ZSINIT frame */
#define ZATTNLEN 32	/* Max length of attention string */
/* Bit Masks for ZSINIT flags byte ZF0 */
#define TESCCTL 0100	/* Transmitter expects ctl chars to be escaped */
#define TESC8   0200	/* Transmitter expects 8th bit to be escaped */

/* Parameters for ZFILE frame */
/* Conversion options one of these in ZF0 */
#define ZCBIN	1	/* Binary transfer - inhibit conversion */
#define ZCNL	2	/* Convert NL to local end of line convention */
#define ZCRESUM	3	/* Resume interrupted file transfer */
/* Management include options, one of these ored in ZF1 */
#define ZF1_ZMSKNOLOC   0x80 /* Skip file if not present at rx */
/* Management options, one of these ored in ZF1 */
#define ZF1_ZMMASK	    0x1f /* Mask for the choices below */
#define ZF1_ZMNEWL         1 /* Transfer if source newer or longer */
#define ZF1_ZMCRC          2 /* Transfer if different file CRC or length */
#define ZF1_ZMAPND         3 /* Append contents to existing file (if any) */
#define ZF1_ZMCLOB         4 /* Replace existing file */
#define ZF1_ZMNEW          5 /* Transfer if source newer */
	/* Number 5 is alive ... */
#define ZF1_ZMDIFF         6 /* Transfer if dates or lengths different */
#define ZF1_ZMPROT         7 /* Protect destination file */
#define ZF1_ZMCHNG         8 /* Change filename if destination exists */

/* Transport options, one of these in ZF2 */
#define ZTLZW	1	/* Lempel-Ziv compression */
#define ZTCRYPT	2	/* Encryption */
#define ZTRLE	3	/* Run Length encoding */
/* Extended options for ZF3, bit encoded */
#define ZXSPARS	64	/* Encoding for sparse file operations */

/* Parameters for ZCOMMAND frame ZF0 (otherwise 0) */
#define ZCACK1	1	/* Acknowledge, then do command */

/******************************
 *enum
 *****************************/
typedef enum{
    STATE_IDLE = 0,
    STATE_CHK_ENC,
    STATE_PARSE_HEX,
    STATE_PARSE_BIN,
    STATE_PARSE_BIN32,
    STATE_PARSE_LINESEEDXON,
    
    STATE_PARSE_FILE_NAME,
    STATE_PARSE_FILE_SIZE,
    STATE_PARSE_FILE_MTIME,
    STATE_PARSE_FILE_MODE,
    STATE_PARSE_FILE_NFILELEFT,
    STATE_PARSE_FILE_TOTALSIZELEFT,
    
    STATE_ZRQINIT,

    STATE_EXIT,
    STATE_DUMP
} ZmodemState;

typedef enum{
    ZR_ERROR  = -1,
    ZR_DONE   = 0,
    ZR_PARTLY = 1
} ZmodemResult;

/******************************
 *struct
 *****************************/

#pragma   pack(1)


struct enc_header_tag{
    char      hex_pre[4];
};
typedef struct enc_header_tag enc_header_t;

struct hex_str_tag{
    char hex[2];
};
typedef struct hex_str_tag hex_str_t;    
struct hex_tag{
    hex_str_t type;
    hex_str_t flag[4];
    hex_str_t crc[2];         
};
typedef struct hex_tag hex_t;

struct frame_tag{
    unsigned char type;
    unsigned char flag[4];
    unsigned short crc;
};
typedef struct frame_tag frame_t;

struct frame32_tag{
    unsigned char type;
    unsigned char flag[4];
	unsigned long crc;
};
typedef struct frame32_tag frame32_t;

struct lineseed_tag{
    char lineseed[2];         
};
typedef struct lineseed_tag lineseed_t;
struct lineseedxon_tag{
    char lineseed[2]; 
    char xon;
};
typedef struct lineseedxon_tag lineseedxon_t;

#pragma   pack()



#endif /* ZMODEM_H */

