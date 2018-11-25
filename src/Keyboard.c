/******************************************************************************/
/* src/Keyboard.c                                                             */
/*                                                                 2018/10/10 */
/* Copyright (C) 2018 Mochi.                                                  */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 共通ヘッダ */
#include <mtty.h>
#include <stdbool.h>
#include <string.h>
#include <kernel/library.h>
#include <MLib/Basic/MLibBasic.h>

/* モジュールヘッダ */
#include "mterm.h"
#include "Screen.h"


/******************************************************************************/
/* 定義                                                                       */
/******************************************************************************/
/* スキャンコード */
#define SCANCODE_ALT_LEFT_ON     ( 0x380000 )   /** 左altキーON    */
#define SCANCODE_ALT_LEFT_OFF    ( 0xB80000 )   /** 左altキーOFF   */
#define SCANCODE_ALT_RIGHT_ON    ( 0xE03800 )   /** 右altキーON    */
#define SCANCODE_ALT_RIGHT_OFF   ( 0xE0B800 )   /** 右altキーOFF   */
#define SCANCODE_CTRL_LEFT_ON    ( 0x1D0000 )   /** 左ctrlキーON   */
#define SCANCODE_CTRL_LEFT_OFF   ( 0x9D0000 )   /** 左ctrlキーOFF  */
#define SCANCODE_CTRL_RIGHT_ON   ( 0xE01D00 )   /** 右ctrlキーON   */
#define SCANCODE_CTRL_RIGHT_OFF  ( 0xE09D00 )   /** 右ctrlキーOFF  */
#define SCANCODE_SHIFT_LEFT_ON   ( 0x2A0000 )   /** 左shiftキーON  */
#define SCANCODE_SHIFT_LEFT_OFF  ( 0xAA0000 )   /** 左shiftキーOFF */
#define SCANCODE_SHIFT_RIGHT_ON  ( 0x360000 )   /** 右shiftキーON  */
#define SCANCODE_SHIFT_RIGHT_OFF ( 0xB60000 )   /** 右shiftキーOFF */

/* 修飾キー状態ビットマスク */
#define MODIFIER_MASK_ALT_LEFT    ( 0x00000001 )    /** 左altビットマスク   */
#define MODIFIER_MASK_ALT_RIGHT   ( 0x00000002 )    /** 右altビットマスク   */
#define MODIFIER_MASK_ALT         ( 0x00000003 )    /** altビットマスク     */
#define MODIFIER_MASK_CTRL_LEFT   ( 0x00000004 )    /** 左ctrlビットマスク  */
#define MODIFIER_MASK_CTRL_RIGHT  ( 0x00000008 )    /** 右ctrlビットマスク  */
#define MODIFIER_MASK_CTRL        ( 0x0000000C )    /** ctrlビットマスク    */
#define MODIFIER_MASK_SHIFT_LEFT  ( 0x00000010 )    /** 左shiftビットマスク */
#define MODIFIER_MASK_SHIFT_RIGHT ( 0x00000020 )    /** 右shiftビットマスク */
#define MODIFIER_MASK_SHIFT       ( 0x00000030 )    /** shiftビットマスク   */

/** 変換テーブルエントリ型 */
typedef struct {
    uint32_t scan;      /**< スキャンコード */
    char     *pUnshift; /**< 通常コード     */
    char     *pShift;   /**< shiftコード    */
    char     *pCtrl;    /**< ctrlコード     */
    bool     alt;       /**< alt有効無効    */
} ConvEntry_t;

/** 修飾キー状態設定規則エントリ型 */
typedef struct {
    uint32_t scan;      /**< スキャンコード           */
    uint32_t mask;      /**< 修飾キー状態ビットマスク */
    bool     state;     /**< 設定状態                 */
} ModifierRuleEntry_t;


/******************************************************************************/
/* ローカル関数宣言                                                           */
/******************************************************************************/
/* スキャンコード変換 */
static void Convert( uint32_t scan );

/* 修飾キー処理 */
static bool ProcModifierKey( uint32_t scan );

/* コード送信 */
static void SendCode( char *pCode,
                      bool alt     );


/******************************************************************************/
/* グローバル変数                                                             */
/******************************************************************************/
/** 変換テーブル */
const static ConvEntry_t gConvTbl[] = {
    /*-------+---------+---------+---------+---------+-------*/
    /* print | scan    | unshift | shift   | ctrl    | alt   */
    /*-------+---------+---------+---------+---------+-------*/
    /* ESC */{ 0x010000, "\e"    , "\e"    , NULL    , true  },
    /* 1!  */{ 0x020000, "1"     , "!"     , NULL    , true  },
    /* 2"  */{ 0x030000, "2"     , "\""    , "\x00"  , true  },
    /* 3#  */{ 0x040000, "3"     , "#"     , "\x1B"  , true  },
    /* 4$  */{ 0x050000, "4"     , "$"     , "\x1C"  , true  },
    /* 5%  */{ 0x060000, "5"     , "%"     , "\x1D"  , true  },
    /* 6&  */{ 0x070000, "6"     , "&"     , "\x1E"  , true  },
    /* 7'  */{ 0x080000, "7"     , "'"     , "\x1F"  , true  },
    /* 8(  */{ 0x090000, "8"     , "("     , "\x7F"  , true  },
    /* 9)  */{ 0x0A0000, "9"     , ")"     , NULL    , true  },
    /* 0   */{ 0x0B0000, "0"     , ""      , NULL    , true  },
    /* -=  */{ 0x0C0000, "-"     , "="     , NULL    , true  },
    /* ^~  */{ 0x0D0000, "^"     , "~"     , NULL    , true  },
    /* BS  */{ 0x0E0000, "\b"    , "\b"    , "\x08"  , true  },
    /* TAB */{ 0x0F0000, "\t"    , "\t"    , NULL    , true  },
    /* qQ  */{ 0x100000, "q"     , "Q"     , "\x11"  , true  },
    /* wW  */{ 0x110000, "w"     , "W"     , "\x17"  , true  },
    /* eE  */{ 0x120000, "e"     , "E"     , "\x05"  , true  },
    /* rR  */{ 0x130000, "r"     , "R"     , "\x12"  , true  },
    /* tT  */{ 0x140000, "t"     , "T"     , "\x14"  , true  },
    /* yY  */{ 0x150000, "y"     , "Y"     , "\x19"  , true  },
    /* uU  */{ 0x160000, "u"     , "U"     , "\x15"  , true  },
    /* iI  */{ 0x170000, "i"     , "I"     , "\x09"  , true  },
    /* oO  */{ 0x180000, "o"     , "O"     , "\x0F"  , true  },
    /* pP  */{ 0x190000, "p"     , "P"     , "\x10"  , true  },
    /* @`  */{ 0x1A0000, "@"     , "`"     , "\x00"  , true  },
    /* [{  */{ 0x1B0000, "["     , "{"     , "\x1B"  , true  },
    /* ETR */{ 0x1C0000, "\n"    , "\n"    , NULL    , true  },
    /* aA  */{ 0x1E0000, "a"     , "A"     , "\x01"  , true  },
    /* sS  */{ 0x1F0000, "s"     , "S"     , "\x13"  , true  },
    /* dD  */{ 0x200000, "d"     , "D"     , "\x04"  , true  },
    /* fF  */{ 0x210000, "f"     , "F"     , "\x06"  , true  },
    /* gG  */{ 0x220000, "g"     , "G"     , "\x07"  , true  },
    /* hH  */{ 0x230000, "h"     , "H"     , "\x08"  , true  },
    /* jJ  */{ 0x240000, "j"     , "J"     , "\x0A"  , true  },
    /* kK  */{ 0x250000, "k"     , "K"     , "\x0B"  , true  },
    /* lL  */{ 0x260000, "l"     , "L"     , "\x0C"  , true  },
    /* ;+  */{ 0x270000, ";"     , "+"     , NULL    , true  },
    /* :*  */{ 0x280000, ":"     , "*"     , NULL    , true  },
    /* ]}  */{ 0x2B0000, "]"     , "}"     , "\x1D"  , true  },
    /* zZ  */{ 0x2C0000, "z"     , "Z"     , "\x1A"  , true  },
    /* xX  */{ 0x2D0000, "x"     , "X"     , "\x18"  , true  },
    /* cC  */{ 0x2E0000, "c"     , "C"     , "\x03"  , true  },
    /* vV  */{ 0x2F0000, "v"     , "V"     , "\x16"  , true  },
    /* bB  */{ 0x300000, "b"     , "B"     , "\x02"  , true  },
    /* nN  */{ 0x310000, "n"     , "N"     , "\x0E"  , true  },
    /* mM  */{ 0x320000, "m"     , "M"     , "\x0D"  , true  },
    /* ,<  */{ 0x330000, ","     , "<"     , NULL    , true  },
    /* .>  */{ 0x340000, "."     , ">"     , NULL    , true  },
    /* /?  */{ 0x350000, "/"     , "?"     , "\x1F"  , true  },
    /* TK* */{ 0x370000, "*"     , "*"     , NULL    , true  },
    /* SPC */{ 0x390000, " "     , " "     , "\x00"  , true  },
    /* F1  */{ 0x3B0000, "\e[11~", "\e[25~", NULL    , false },
    /* F2  */{ 0x3C0000, "\e[12~", "\e[26~", NULL    , false },
    /* F3  */{ 0x3D0000, "\e[13~", "\e[28~", NULL    , false },
    /* F4  */{ 0x3E0000, "\e[14~", "\e[29~", NULL    , false },
    /* F5  */{ 0x3F0000, "\e[15~", "\e[31~", NULL    , false },
    /* F6  */{ 0x400000, "\e[17~", "\e[32~", NULL    , false },
    /* F7  */{ 0x410000, "\e[18~", "\e[33~", NULL    , false },
    /* F8  */{ 0x420000, "\e[19~", "\e[34~", NULL    , false },
    /* F9  */{ 0x430000, "\e[20~", NULL    , NULL    , false },
    /* F10 */{ 0x440000, "\e[21~", NULL    , NULL    , false },
    /* TK7 */{ 0x470000, "7"     , "7"     , NULL    , false },
    /* TK8 */{ 0x480000, "8"     , "8"     , NULL    , false },
    /* TK9 */{ 0x490000, "9"     , "9"     , NULL    , false },
    /* TK- */{ 0x4A0000, "-"     , "-"     , NULL    , false },
    /* TK4 */{ 0x4B0000, "4"     , "4"     , NULL    , false },
    /* TK5 */{ 0x4C0000, "5"     , "5"     , NULL    , false },
    /* TK6 */{ 0x4D0000, "6"     , "6"     , NULL    , false },
    /* TK+ */{ 0x4E0000, "+"     , "+"     , NULL    , false },
    /* TK1 */{ 0x4F0000, "1"     , "1"     , NULL    , false },
    /* TK2 */{ 0x500000, "2"     , "2"     , NULL    , false },
    /* TK3 */{ 0x510000, "3"     , "3"     , NULL    , false },
    /* TK0 */{ 0x520000, "0"     , "0"     , NULL    , false },
    /* TK. */{ 0x530000, "."     , "."     , NULL    , false },
    /* F11 */{ 0x570000, "\e[23~", NULL    , NULL    , false },
    /* F12 */{ 0x580000, "\e[24~", NULL    , NULL    , false },
    /* \_  */{ 0x730000, "\\"    , "_"     , "\x1C"  , true  },
    /* \|  */{ 0x7D0000, "\\"    , "|"     , "\x1C"  , true  },
    /* TKE */{ 0xE01C00, "\n"    , "\n"    , NULL    , false },
    /* TK/ */{ 0xE03500, "/"     , "/"     , NULL    , false },
    /* HOME*/{ 0xE04700, "\e[1~" , "\e[1~" , NULL    , false },
    /* ↑  */{ 0xE04800, "\e[A"  , "\e[A"  , NULL    , false },
    /* PgUp*/{ 0xE04900, "\e[5~" , "\e[5~" , NULL    , false },
    /* ←  */{ 0xE04B00, "\e[D"  , "\e[D"  , NULL    , false },
    /* →  */{ 0xE04D00, "\e[C"  , "\e[C"  , NULL    , false },
    /* END */{ 0xE04F00, "\e[4~" , "\e[4~" , NULL    , false },
    /* ↓  */{ 0xE05000, "\e[B"  , "\e[B"  , NULL    , false },
    /* PgDn*/{ 0xE05100, "\e[6~" , "\e[6~" , NULL    , false },
    /* INS */{ 0xE05200, "\e[2~" , "\e[2~" , NULL    , false },
    /* DEL */{ 0xE05300, "\x7F"  , "\x7F"  , NULL    , false }  };
    /*-------+---------+---------+---------+---------+-------*/
    /* print | scan    | unshift | shift   | ctrl    | alt   */
    /*-------+---------+---------+---------+---------+-------*/

/** 修飾キー状態設定規則テーブル */
const static ModifierRuleEntry_t gModifierRuleTbl[] = {
    /*------------------------+--------------------------+-------*/
    /* scan                   | bit mask                 | state */
    /*------------------------+--------------------------+-------*/
    { SCANCODE_ALT_LEFT_ON    , MODIFIER_MASK_ALT_LEFT   , true  },
    { SCANCODE_ALT_LEFT_OFF   , MODIFIER_MASK_ALT_LEFT   , false },
    { SCANCODE_ALT_RIGHT_ON   , MODIFIER_MASK_ALT_RIGHT  , true  },
    { SCANCODE_ALT_RIGHT_OFF  , MODIFIER_MASK_ALT_RIGHT  , false },
    { SCANCODE_CTRL_LEFT_ON   , MODIFIER_MASK_CTRL_LEFT  , true  },
    { SCANCODE_CTRL_LEFT_OFF  , MODIFIER_MASK_CTRL_LEFT  , false },
    { SCANCODE_CTRL_RIGHT_ON  , MODIFIER_MASK_CTRL_RIGHT , true  },
    { SCANCODE_CTRL_RIGHT_OFF , MODIFIER_MASK_CTRL_RIGHT , false },
    { SCANCODE_SHIFT_LEFT_ON  , MODIFIER_MASK_SHIFT_LEFT , true  },
    { SCANCODE_SHIFT_LEFT_OFF , MODIFIER_MASK_SHIFT_LEFT , false },
    { SCANCODE_SHIFT_RIGHT_ON , MODIFIER_MASK_SHIFT_RIGHT, true  },
    { SCANCODE_SHIFT_RIGHT_OFF, MODIFIER_MASK_SHIFT_RIGHT, false }  };
    /*------------------------+--------------------------+-------*/

/** 修飾キー状態 */
static uint32_t gModifierState = 0;


/******************************************************************************/
/* グローバル関数定義                                                         */
/******************************************************************************/
void KeyboardInput( MtermMsgInput_t *pMsg )
{
    bool ret;
    
    /* 修飾キー処理 */
    ret = ProcModifierKey( pMsg->scan );
    
    /* 処理実施判定 */
    if ( ret != false ) {
        /* 処理実施 */
        
        return;
    }
    
    /* スキャンコード変換 */
    Convert( pMsg->scan );
    
    return;
}


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
/******************************************************************************/
/**
 * @brief       スキャンコード変換
 * @details     スキャンコードを変換テーブルを基に変換し、コードを送信する。
 * 
 * @param[in]   scan スキャンコード
 */
/******************************************************************************/
static void Convert( uint32_t scan )
{
    uint32_t index;
    
    /* 変換テーブルエントリ毎に繰り返し */
    for ( index = 0; index < MLIB_BASIC_NUMOF( gConvTbl ); index++ ) {
        /* スキャンコード判定 */
        if ( gConvTbl[ index ].scan != scan ) {
            /* 不一致 */
            
            /* 次エントリ */
            continue;
        }
        
        /* ctrl/shift修飾キー状態判定 */
        if ( ( gModifierState & MODIFIER_MASK_CTRL ) != false ) {
            /* ctrl on */
            
            /* ctrlコード送信 */
            SendCode( gConvTbl[ index ].pCtrl,
                      gConvTbl[ index ].alt    );
            
        } else if ( ( gModifierState & MODIFIER_MASK_SHIFT ) != false ) {
            /* shift on */
            
            /* shiftコード送信 */
            SendCode( gConvTbl[ index ].pShift,
                      gConvTbl[ index ].alt     );
            
        } else {
            /* 修飾なし */
            
            /* 通常コード送信 */
            SendCode( gConvTbl[ index ].pUnshift,
                      gConvTbl[ index ].alt       );
        }
        
        return;
    }
    
    return;
}


/******************************************************************************/
/**
 * @brief       修飾キー処理
 * @details     修飾キーのスキャンコードか判定し、修飾キーの場合は修飾キー状態
 *              を設定する。
 * 
 * @param[in]   scan スキャンコード
 * 
 * @return      修飾キー処理を行ったかを返す。
 * @retval      true  処理実施
 * @retval      flase 未処理(修飾キーでない)
 */
/******************************************************************************/
static bool ProcModifierKey( uint32_t scan )
{
    uint32_t index;
    
    /* 修飾キー状態設定規則テーブルエントリ毎に繰り返し */
    for ( index = 0; index < MLIB_BASIC_NUMOF( gModifierRuleTbl ); index++ ) {
        /* 修飾キー判定 */
        if ( gModifierRuleTbl[ index ].scan != scan ) {
            /* 不一致 */
            
            /* 次エントリ */
            continue;
        }
        
        /* 設定状態判定 */
        if ( gModifierRuleTbl[ index ].state == false ) {
            /* キーOFF */
            
            /* 修飾キー状態設定 */
            gModifierState &= ~gModifierRuleTbl[ index ].mask;
            
        } else {
            /* キーON */
            
            /* 修飾キー状態設定 */
            gModifierState |= gModifierRuleTbl[ index ].mask;
        }
        
        return true;
    }
    
    return false;
}


/******************************************************************************/
/**
 * @brief       コード送信
 * @details     コードをTTYに送信する。alt修飾が有効でかつaltキーがON状態の場合
 *              はコードの前にESCコードを付加して送信する。
 * 
 * @param[in]   *pCode コード
 * @param[in]   alt    alt修飾有効無効
 *                  - true  有効
 *                  - false 無効
 */
/******************************************************************************/
static void SendCode( char *pCode,
                      bool alt     )
{
    char        buffer[ 20 ];
    size_t      length;
    MttyMsgInput_t *pMsg;
    
    /* 初期化 */
    length = 0;
    pMsg   = ( MttyMsgInput_t * ) buffer;
    
    /* 変換コードチェック */
    if ( pCode == NULL ) {
       /* 変換無し */
       
       return;
    }
    
    /* コード長計算 */
    length = strlen( pCode );
    
    /* コード長判定 */
    if ( length == 0 ) {
        /* コード長0 */
        
        /* コード長補正 */
        length = 1;
    }
    
    /* alt修飾判定 */
    if ( ( alt                                    != false ) &&
         ( ( gModifierState & MODIFIER_MASK_ALT ) != false )    ) {
        /* alt修飾有 */
        
        /* ESCコード設定 */
        pMsg->data[ 0 ] = '\e';
        length++;
        
        /* コード設定 */
        strcpy( &( pMsg->data[ 1 ] ), pCode );
        
    } else {
        /* alt修飾無 */
        
        strcpy( pMsg->data, pCode );
    }
    
    /* メッセージ設定 */
    pMsg->header.funcId = MTTY_FUNC_INPUT;
    pMsg->header.length = length;
    
    /* コード送信 */
    MkMsgSend( 4, pMsg, sizeof ( pMsg ) + length, NULL );
    
    return;
}


/******************************************************************************/
