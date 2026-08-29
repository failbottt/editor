#ifndef KEYDEFS_H
#define KEYDEFS_H

/* https://vt100.net/docs/vt100-ug/chapter3.html */
/* F1-FN aren't supported till vt200-300, however F1-F5 aren't sent because they would have been host function keys that did not get sent to the mainframe */

#define CTRL_SPACE               0x00  /* NUL */

#define CTRL_A                   0x01  /* SOH */
#define CTRL_B                   0x02  /* STX */
#define CTRL_C                   0x03  /* ETX */
#define CTRL_D                   0x04  /* EOT */
#define CTRL_E                   0x05  /* ENQ */
#define CTRL_F                   0x06  /* ACK */
#define CTRL_G                   0x07  /* BELL */
#define CTRL_H                   0x08  /* BS */
#define KEY_BACKSPACE            0x08  /* Backspace function */
#define CTRL_I                   0x09  /* HT */
#define KEY_TAB                  0x09  /* Tab function */
#define CTRL_J                   0x0A  /* LF */
#define KEY_LINEFEED             0x0A  /* Line Feed */
#define CTRL_K                   0x0B  /* VT */
#define CTRL_L                   0x0C  /* FF */
#define CTRL_M                   0x0D  /* CR */
#define KEY_RETURN               0x0D  /* Carriage return function */
#define CTRL_N                   0x0E  /* SO */
#define CTRL_O                   0x0F  /* SI */
#define CTRL_P                   0x10  /* DLE */
#define CTRL_Q                   0x11  /* DC1 / XON */
#define CTRL_R                   0x12  /* DC2 */
#define CTRL_S                   0x13  /* DC3 / XOFF */
#define CTRL_T                   0x14  /* DC4 */
#define CTRL_U                   0x15  /* NAK */
#define CTRL_V                   0x16  /* SYN */
#define CTRL_W                   0x17  /* ETB */
#define CTRL_X                   0x18  /* CAN */
#define CTRL_Y                   0x19  /* EM */
#define CTRL_Z                   0x1A  /* SUB */
#define CTRL_LEFT_BRACKET        0x1B  /* ESC */
#define KEY_ESCAPE               0x1B  /* Initial delimiter of an escape sequence */
#define CTRL_BACKSLASH           0x1C  /* FS */
#define CTRL_RIGHT_BRACKET       0x1D  /* GS */
#define CTRL_TILDE               0x1E  /* RS */
#define CTRL_QUESTION_MARK       0x1F  /* US */

#define KEY_SPACE                0x20  /* Space */
#define KEY_SHIFT_1              0x21  /* ! */
#define KEY_SHIFT_APOSTROPHE     0x22  /* \" */
#define KEY_SHIFT_3              0x23  /* # / £ */
#define KEY_SHIFT_4              0x24  /* $ */
#define KEY_SHIFT_5              0x25  /* % */
#define KEY_SHIFT_7              0x26  /* & */
#define KEY_APOSTROPHE           0x27  /* \' */
#define KEY_SHIFT_9              0x28  /* ( */
#define KEY_SHIFT_0              0x29  /* ) */
#define KEY_SHIFT_8              0x2A  /* * */
#define KEY_SHIFT_EQUALS         0x2B  /* + */
#define KEY_COMMA                0x2C  /* , */
#define KEY_MINUS                0x2D  /* - */
#define KEY_PERIOD               0x2E  /* . */
#define KEY_SLASH                0x2F  /* / */

#define KEY_0                    0x30  /* 0 */
#define KEY_1                    0x31  /* 1 */
#define KEY_2                    0x32  /* 2 */
#define KEY_3                    0x33  /* 3 */
#define KEY_4                    0x34  /* 4 */
#define KEY_5                    0x35  /* 5 */
#define KEY_6                    0x36  /* 6 */
#define KEY_7                    0x37  /* 7 */
#define KEY_8                    0x38  /* 8 */
#define KEY_9                    0x39  /* 9 */
#define KEY_SHIFT_SEMICOLON      0x3A  /* : */
#define KEY_SEMICOLON            0x3B  /* ; */
#define KEY_SHIFT_COMMA          0x3C  /* < */
#define KEY_EQUALS               0x3D  /* = */
#define KEY_SHIFT_PERIOD         0x3E  /* > */
#define KEY_SHIFT_SLASH          0x3F  /* ? */

#define KEY_SHIFT_2              0x40  /* @ */

#define KEY_A_UPPER              0x41  /* A */
#define KEY_B_UPPER              0x42  /* B */
#define KEY_C_UPPER              0x43  /* C */
#define KEY_D_UPPER              0x44  /* D */
#define KEY_E_UPPER              0x45  /* E */
#define KEY_F_UPPER              0x46  /* F */
#define KEY_G_UPPER              0x47  /* G */
#define KEY_H_UPPER              0x48  /* H */
#define KEY_I_UPPER              0x49  /* I */
#define KEY_J_UPPER              0x4A  /* J */
#define KEY_K_UPPER              0x4B  /* K */
#define KEY_L_UPPER              0x4C  /* L */
#define KEY_M_UPPER              0x4D  /* M */
#define KEY_N_UPPER              0x4E  /* N */
#define KEY_O_UPPER              0x4F  /* O */
#define KEY_P_UPPER              0x50  /* P */
#define KEY_Q_UPPER              0x51  /* Q */
#define KEY_R_UPPER              0x52  /* R */
#define KEY_S_UPPER              0x53  /* S */
#define KEY_T_UPPER              0x54  /* T */
#define KEY_U_UPPER              0x55  /* U */
#define KEY_V_UPPER              0x56  /* V */
#define KEY_W_UPPER              0x57  /* W */
#define KEY_X_UPPER              0x58  /* X */
#define KEY_Y_UPPER              0x59  /* Y */
#define KEY_Z_UPPER              0x5A  /* Z */

#define KEY_LEFT_BRACKET         0x5B  /* [ */
#define KEY_BACKSLASH            0x5C  /* backslash */
#define KEY_RIGHT_BRACKET        0x5D  /* ] */
#define KEY_SHIFT_6              0x5E  /* ^ */
#define KEY_SHIFT_MINUS          0x5F  /* _ */

#define KEY_BACKTICK             0x60  /* ` */

#define KEY_A_LOWER              0x61  /* a */
#define KEY_B_LOWER              0x62  /* b */
#define KEY_C_LOWER              0x63  /* c */
#define KEY_D_LOWER              0x64  /* d */
#define KEY_E_LOWER              0x65  /* e */
#define KEY_F_LOWER              0x66  /* f */
#define KEY_G_LOWER              0x67  /* g */
#define KEY_H_LOWER              0x68  /* h */
#define KEY_I_LOWER              0x69  /* i */
#define KEY_J_LOWER              0x6A  /* j */
#define KEY_K_LOWER              0x6B  /* k */
#define KEY_L_LOWER              0x6C  /* l */
#define KEY_M_LOWER              0x6D  /* m */
#define KEY_N_LOWER              0x6E  /* n */
#define KEY_O_LOWER              0x6F  /* o */
#define KEY_P_LOWER              0x70  /* p */
#define KEY_Q_LOWER              0x71  /* q */
#define KEY_R_LOWER              0x72  /* r */
#define KEY_S_LOWER              0x73  /* s */
#define KEY_T_LOWER              0x74  /* t */
#define KEY_U_LOWER              0x75  /* u */
#define KEY_V_LOWER              0x76  /* v */
#define KEY_W_LOWER              0x77  /* w */
#define KEY_X_LOWER              0x78  /* x */
#define KEY_Y_LOWER              0x79  /* y */
#define KEY_Z_LOWER              0x7A  /* z */

#define KEY_SHIFT_LEFT_BRACKET   0x7B  /* { */
#define KEY_SHIFT_BACKSLASH      0x7C  /* | */
#define KEY_SHIFT_RIGHT_BRACKET  0x7D  /* } */
#define KEY_SHIFT_BACKTICK       0x7E  /* ~ */

#define KEY_DELETE               0x7F  /* Ignored by the VT100 */
 
#endif
