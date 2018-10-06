/******************************************************************************/
/* src/main.c                                                                 */
/*                                                                 2018/09/26 */
/* Copyright (C) 2018 Mochi.                                                  */
/******************************************************************************/
/******************************************************************************/
/* インクルード                                                               */
/******************************************************************************/
/* 共通ヘッダ */
#include <stdbool.h>
#include "mterm.h"
#include <kernel/library.h>

/* モジュールヘッダ */
#include "Keyboard.h"
#include "Screen.h"


/******************************************************************************/
/* ローカル関数定義                                                           */
/******************************************************************************/
void main( void )
{
    char          buffer[ MK_MSG_SIZE_MAX + 1 ];/* 受信メッセージバッファ */
    int32_t       size;                         /* 受信メッセージサイズ   */
    uint32_t      errNo;                        /* エラー番号             */
    MtermMsgHdr_t *pMsg;                        /* メッセージ             */
    
    /* 初期化 */
    errNo = MK_MSG_ERR_NONE;
    pMsg  = ( MtermMsgHdr_t * ) buffer;
    
    /* 画面初期化 */
    ScreenInit();
    
    /* メインループ */
    while ( true ) {
        /* メッセージ受信 */
        size = MkMsgReceive( MK_CONFIG_TASKID_NULL,     /* タスクID           */
                             pMsg,                      /* メッセージバッファ */
                             MK_MSG_SIZE_MAX,           /* バッファサイズ     */
                             &errNo                );   /* エラー番号         */
        
        /* メッセージ受信結果判定 */
        if ( size == MK_MSG_RET_FAILURE ) {
            /* 失敗 */
            
            continue;
        }
        
        /* 機能ID判定 */
        if ( pMsg->funcId == MTERM_FUNC_OUTPUT ) {
            /* 出力 */
            
            ScreenOutput( ( MtermMsgOutput_t * ) pMsg );
            
        } else if ( pMsg->funcId == MTERM_FUNC_INPUT ) {
            /* 入力 */
            
            KeyboardInput( ( MtermMsgInput_t * ) pMsg );
        }
    }
}


/******************************************************************************/
