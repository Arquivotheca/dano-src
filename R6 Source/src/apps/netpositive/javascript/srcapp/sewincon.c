/* sewincon.c     Main window and utilities for Windows version
 */

/* (c) COPYRIGHT 1993-98           NOMBAS, INC.
 *                                 64 SALEM ST.
 *                                 MEDFORD, MA 02155  USA
 * 
 * ALL RIGHTS RESERVED
 * 
 * This software is the property of Nombas, Inc. and is furnished under
 * license by Nombas, Inc.; this software may be used only in accordance
 * with the terms of said license.  This copyright notice may not be removed,
 * modified or obliterated without the prior written permission of Nombas, Inc.
 * 
 * This software is a Trade Secret of Nombas, Inc.
 * 
 * This software may not be copied, transmitted, provided to or otherwise made
 * available to any other person, company, corporation or other entity except
 * as specified in the terms of said license.
 * 
 * No right, title, ownership or other interest in the software is hereby
 * granted or transferred.
 * 
 * The information contained herein is subject to change without notice and
 * should not be construed as a commitment by Nombas, Inc.
 */

#include "jseopt.h"

#if defined(__JSE_WIN16__) || defined(__JSE_WIN32__) || (defined(__JSE_CON32__) && !defined(__CGI__))

#if defined(__NPEXE__)
   #include "jseshim.h"
#endif

#define MIN_X_SIZE 1
#define MIN_Y_SIZE 1


#if defined(JSE_WINDOW)

#if defined(JSE_SCREEN_SETFOREGROUND) || \
    defined(JSE_SCREEN_SETBACKGROUND)
   COLORREF
jseContextAndThreeIntsToRGB(jseContext jsecontext)
{
   jseVariable R = jseFuncVarNeed(jsecontext, 0, JSE_VN_NUMBER);
   jseVariable G = jseFuncVarNeed(jsecontext, 1, JSE_VN_NUMBER);
   jseVariable B = jseFuncVarNeed(jsecontext, 2, JSE_VN_NUMBER);
   int r = (int)jseGetNumber(jsecontext, R);
   int g = (int)jseGetNumber(jsecontext, G);
   int b = (int)jseGetNumber(jsecontext, B);
   return RGB(r,g,b);
}
#endif

#if defined(JSE_SCREEN_SETFOREGROUND)
jseLibFunc(ScreenSetForegrnd)
{
   jseMainWindowFromContext(jsecontext)->Foreground = jseContextAndThreeIntsToRGB(jsecontext);
}
#endif

#if defined(JSE_SCREEN_SETBACKGROUND)
jseLibFunc(ScreenSetBackgrnd)
{
   COLORREF Background = jseContextAndThreeIntsToRGB(jsecontext);
   HBRUSH hBrush = CreateSolidBrush(Background);
   struct jseMainWindow *MainWin = jseMainWindowFromContext(jsecontext);
   MainWin->hBrush = hBrush;
   MainWin->Background = Background;
   #if defined(__JSE_WIN16__)
      SetClassLong(MainWin->hWindow,GCW_HBRBACKGROUND,(WORD)hBrush);
   #else
      SetClassLong(MainWin->hWindow,GCL_HBRBACKGROUND,(LONG)hBrush);
   #endif
}
#endif


#define DEFAULT_SCREEN_HEIGHT 25
#define DEFAULT_SCREEN_WIDTH  80
#define DEFAULT_MAX_LINES     1000 /* how many rows to keep in memory */

#define SELECT_TEXT_TIMER_ID (SUSPEND_TIMER_ID+1)
#define SELECT_TEXT_INTERVAL 20

#if defined(__JSE_WIN16__)
   VAR_DATA(uint) jsemainwindowCallbackDepth = 0;
   /* variable_data ok because only used in win16 */
#endif

#if !defined(__CGI__)
   CONST_DATA(jsechar) UnprintableChars[] = UNISTR("\n\r\t\b\a\032\033");
#endif

#define IS_KEY_DOWN(V_KEYCODE)  ( GetKeyState(V_KEYCODE) < 0 )

#if !defined(__CGI__)
   static void NEAR_CALL
jsemainwindowWmSize(struct jseMainWindow *This,WPARAM wParam)
{
   int x,y;

   /* If we are being hidden (or iconized), we don't worry about this message */

   if ( wParam==SIZENORMAL || wParam==SIZEFULLSCREEN || wParam==SIZEZOOMSHOW )
   {
      RECT rect;

      /* I used to extract the values from lParam, but that failed in the Netscape plug-in */
      GetClientRect(This->hWindow,&rect);
      This->cxClient = rect.right - rect.left;
      This->cyClient = rect.bottom - rect.top;

      x = This->cxClient/This->cxChar;
      y = This->cyClient/This->cyChar;
      if( x<MIN_X_SIZE ) x = MIN_X_SIZE;
      if( y<MIN_Y_SIZE ) y = MIN_Y_SIZE;

      GetWindowRect(This->hWindow,&rect);

      /* Only 'snap' if the size of the window changed. */
      if ( rect.right-rect.left!=This->cxWindow || rect.bottom-rect.top!=This->cyWindow ) {
         /* no need to resize if the Window is just perfect. */
         if ( x*This->cxChar!=This->cxClient || y*This->cyChar!=This->cyClient )
            jsemainwindowWinSetScreenSize(This,x,y);
      }
      This->scrWidth = x; This->scrHeight = y;
   }
}

   void NEAR_CALL
jsemainwindowWmSetFocus(struct jseMainWindow *This)
{
   CreateCaret(This->hWindow,(HBITMAP)0,This->cxChar,This->cyChar);
   SetCaretPos((This->CursorCol - This->nHscrollPos) * This->cxChar,(This->CursorRow - This->nVscrollPos) * This->cyChar);
   ShowCaret(This->hWindow);
}

   void NEAR_CALL
jsemainwindowWmKillFocus(struct jseMainWindow *This)
{
   HideCaret(This->hWindow);
   DestroyCaret();
}

   static void NEAR_CALL
jsemainwindowWmVScroll(struct jseMainWindow *This,WPARAM wParam,LPARAM lParam)
{
   int nVscrollInc;

   jsebool ThumbAlreadyAtBottom = This->ScrolledToBottom;
   This->ScrolledToBottom = False;

  #ifdef __JSE_WIN32__
   UNUSED_PARAMETER(lParam);
   switch ( LOWORD(wParam) ) {
  #else
   switch (wParam) {
  #endif
      case SB_TOP:
         nVscrollInc = - This->nVscrollPos;
         break;
      case SB_BOTTOM:
         This->ScrolledToBottom = ( 0 != This->nVscrollMax );
         nVscrollInc = This->nVscrollMax - This->nVscrollPos;
         break;
      case SB_LINEUP:
         nVscrollInc = -1;
         break;
      case SB_LINEDOWN:
         nVscrollInc = 1;
         break;
      case SB_PAGEUP:
         nVscrollInc = min(-1,-This->cyClient/This->cyChar);
         break;
      case SB_PAGEDOWN:
         nVscrollInc = max(1,This->cyClient/This->cyChar);
         break;
      case SB_THUMBTRACK:
        #ifdef __JSE_WIN32__
         nVscrollInc = HIWORD(wParam) - This->nVscrollPos;
        #else
         nVscrollInc = LOWORD(lParam) - This->nVscrollPos;
        #endif
         break;
      default:
         nVscrollInc = 0;
         break;
   } /* endswitch */

   if ( 0 != (nVscrollInc = max(-This->nVscrollPos,min(nVscrollInc,This->nVscrollMax-This->nVscrollPos))) ) {
      This->nVscrollPos += nVscrollInc;
#if defined(__JSE_WINCE__)
      ScrollWindowEx(This->hWindow,0,-This->cyChar * nVscrollInc,NULL,NULL,NULL,NULL,SW_INVALIDATE);
#else
      ScrollWindow(This->hWindow,0,-This->cyChar * nVscrollInc,NULL,NULL);
#endif
      SetScrollPos(This->hWindow,SB_VERT,This->nVscrollPos,!(ThumbAlreadyAtBottom && This->ScrolledToBottom));
      jsemainwindowSetVertScroll(This,True);
   } /* endif */
}

   static void NEAR_CALL
jsemainwindowWmHScroll(struct jseMainWindow *This,WPARAM wParam,LONG lParam)
{
   int nHscrollInc;

  #ifdef __JSE_WIN32__
   UNUSED_PARAMETER(lParam);
   switch ( LOWORD(wParam) ) {
  #else
   switch (wParam) {
  #endif
      case SB_LINEUP:
         nHscrollInc = -1;
         break;
      case SB_LINEDOWN:
         nHscrollInc = 1;
         break;
      case SB_PAGEUP:
         nHscrollInc = -8;
         break;
      case SB_PAGEDOWN:
         nHscrollInc = 8;
         break;
      case SB_THUMBTRACK:
        #ifdef __JSE_WIN32__
         nHscrollInc = HIWORD(wParam) - This->nHscrollPos;
        #else
         nHscrollInc = LOWORD(lParam) - This->nHscrollPos;
        #endif
         break;
      default:
         nHscrollInc = 0;
         break;
   } /* endswitch */

   if ( 0 != (nHscrollInc = max(-This->nHscrollPos,min(nHscrollInc,This->nHscrollMax-This->nHscrollPos))) ) {
      This->nHscrollPos += nHscrollInc;
#if defined(__JSE_WINCE__) 
      ScrollWindowEx(This->hWindow,-This->cxChar * nHscrollInc,0,NULL,NULL,NULL,NULL,SW_INVALIDATE);
#else
      ScrollWindow(This->hWindow,-This->cxChar * nHscrollInc,0,NULL,NULL);
#endif
      SetScrollPos(This->hWindow,SB_HORZ,This->nHscrollPos,TRUE);
      UpdateWindow(This->hWindow);
   } /* endif */
}

   static void NEAR_CALL
jsemainwindowWmPaint(struct jseMainWindow *This)
{
   PAINTSTRUCT ps;
   HDC hdc;
   int nPaintBegRow, nPaintRowTooFar, nPaintBegCol, nPaintColTooFar;
   jsechar *buf, *DoubleBlank;
   int DrawLen, BufLen, DrawCol, BlankLen;
   int ColPosition, DrawRow, row;
   
   /* I am not proud of this kludge, but it works. It seems the Window gets
    * repainted twice early, and somehow the second time the invalid region
    * information is lost -> maybe it gets painted blank or something. I don't
    * know. Anyway, this works.
    */
   #if !defined(__NPEXE__)
      if ( This->painted < 2 ) {
         InvalidateRect(This->hWindow,NULL,True);
         This->painted++;
      }
   #endif

   hdc = BeginPaint(This->hWindow,&ps);
   SetBkMode(hdc,TRANSPARENT);
   if ( This->hFont ) {
      SelectObject(hdc,This->hFont);
   } else {
      SelectObject(hdc,GetStockObject(SYSTEM_FIXED_FONT));
   } /* endif */
   SetTextColor(hdc,This->Foreground);

   nPaintBegRow = max(0,This->nVscrollPos + ps.rcPaint.top / This->cyChar);
   nPaintRowTooFar = min(This->NumLines,This->nVscrollPos + ((ps.rcPaint.bottom + This->cyChar - 1) / This->cyChar) );

   nPaintBegCol = max(0,This->nHscrollPos + ps.rcPaint.left / This->cxChar);
   nPaintColTooFar = min(This->scrWidth,This->nHscrollPos + ((ps.rcPaint.right + This->cxChar - 1) / This->cxChar) );

   ColPosition = This->cxChar * (nPaintBegCol - This->nHscrollPos);
   DrawRow = (This->cyChar * (nPaintBegRow - This->nVscrollPos)) - This->tmInternalLeading;
   for ( row = nPaintBegRow; row < nPaintRowTooFar; row++, DrawRow += This->cyChar ) {
      if ( NULL != (buf=This->Data[row]) ) {
         int StrLenBuf;
         DrawLen = nPaintColTooFar-nPaintBegCol;
         StrLenBuf = strlen_jsechar(buf);
         if ( 0 < (BufLen = StrLenBuf - nPaintBegCol) ) {
            if ( 0 != (DrawLen=min(DrawLen,BufLen)) ) {
               jsechar SaveChar;
               int DrawRemaining;
               
               assert( 0 < DrawLen );
               DrawCol = ColPosition;
               buf += nPaintBegCol;
               SaveChar = buf[DrawLen];
               buf[DrawLen] = '\0';
               DrawRemaining = DrawLen;
               do {
                  assert( 0 < DrawRemaining );
                  /* skip any blanks */
                  if ( 0 == (BlankLen = strspn_jsechar(buf,UNISTR(" "))) ) {
                     /* draw up to any blank */
                     if ( NULL == (DoubleBlank = strstr_jsechar(buf,UNISTR("  "))) )
                        BlankLen = DrawRemaining;
                     else
                        BlankLen = DoubleBlank - buf;
                  #if defined(__JSE_WINCE__)
                     ExtTextOut(hdc, DrawCol,DrawRow, (ETO_CLIPPED | ETO_OPAQUE), NULL, buf, BlankLen, NULL); 
                  #else
                     TextOut(hdc,DrawCol,DrawRow,buf,BlankLen);
                  #endif
                  } /* endif */
                  assert( 0 != BlankLen );
                  DrawRemaining -= BlankLen;
                  buf += BlankLen;
                  DrawCol += This->cxChar * BlankLen;
               } while ( 0 != DrawRemaining );
               buf[0] = SaveChar;
               if ( This->Selected ) {
                  /* if selected text is on this row, then invert it */
                  if ( This->SelectedRect.top <= row  &&  row <= This->SelectedRect.bottom ) {
                     int SelectStart, SelectEnd;
                     
                     SelectStart = ( This->SelectedRect.top < row ) ? 0 : This->SelectedRect.left ;
                     if ( SelectStart < nPaintBegCol )
                        SelectStart = nPaintBegCol;
                     SelectEnd = ( row < This->SelectedRect.bottom ) ? MAX_SINT : This->SelectedRect.right ;
                     if ( StrLenBuf < SelectEnd )
                        SelectEnd = StrLenBuf;
                     if ( SelectStart <= SelectEnd ) {
                        RECT rect;
                        rect.bottom = (rect.top = DrawRow + This->tmInternalLeading) + This->cyChar;
                        rect.left = (SelectStart - This->nHscrollPos) * This->cxChar;
                        rect.right = (SelectEnd - This->nHscrollPos) * This->cxChar;
#if !defined(__JSE_WINCE__)
                        InvertRect(hdc,&rect);
#endif
                     } /* endif */
                  } /* endif */
               } /* endif */
            } /* endif */
         } /* endif */
      } /* endif */
   } /* endfor */

   EndPaint(This->hWindow,&ps);

   if ( This->hWindow == GetFocus() ) {
      SetCaretPos((This->CursorCol - This->nHscrollPos) * This->cxChar,(This->CursorRow - This->nVscrollPos) * This->cyChar);
   } /* endif */
}

#if defined(__JSE_WINCE__)
// Windows CE does not handle Global memory allocation, 
// use LocalAlloc, LocalReAlloc, ...
   static void NEAR_CALL
jsemainwindowAddToLocalClipMemory(jsechar *text,int TextLen,HLOCAL *MemHandle,DWORD *TotalSize)
{
   DWORD NewTotal;
   
   assert( 0 != TextLen );
   assert( TextLen <= (int)strlen_jsechar(text) );

   NewTotal = *TotalSize + TextLen;
   *MemHandle = ( *MemHandle )
              ? LocalReAlloc(*MemHandle,sizeof(jsechar)*(NewTotal+1),LMEM_MOVEABLE)
              : LocalAlloc(LPTR,sizeof(jsechar)*(NewTotal+1));
   if ( *MemHandle ) {
      /* lock the memory and copy to it, always with a null at the end */
      jsechar _HUGE_ * mem = (jsechar _HUGE_ *)LocalLock(*MemHandle);
      memcpy(mem+(*TotalSize),text,sizeof(jsechar)*TextLen);
      mem[*TotalSize = NewTotal] = 0;
      LocalUnlock(*MemHandle);
   } /* endif */
}
#else
   static void NEAR_CALL
jsemainwindowAddToGlobalClipMemory(jsechar *text,int TextLen,HGLOBAL *MemHandle,DWORD *TotalSize)
{
   DWORD NewTotal;
   
   assert( 0 != TextLen );
   assert( TextLen <= (int)strlen_jsechar(text) );

   NewTotal = *TotalSize + TextLen;
   *MemHandle = ( *MemHandle )
              ? GlobalReAlloc(*MemHandle,sizeof(jsechar)*(NewTotal+1),GMEM_MOVEABLE)
              : GlobalAlloc(GMEM_MOVEABLE,sizeof(jsechar)*(NewTotal+1));
   if ( *MemHandle ) {
      /* lock the memory and copy to it, always with a null at the end */
      jsechar _HUGE_ * mem = (jsechar _HUGE_ *)GlobalLock(*MemHandle);
      memcpy(mem+(*TotalSize),text,sizeof(jsechar)*TextLen);
      mem[*TotalSize = NewTotal] = 0;
      GlobalUnlock(*MemHandle);
   } /* endif */
}
#endif /* __JSE_WINCE__ */

   static void NEAR_CALL
jsemainwindowCopySelectedTextToClipboard(struct jseMainWindow *This)
{
   DWORD TotalClipSize = 0;
   HGLOBAL GlobalMemHandle = 0;
#if defined(__JSE_WINCE__)
   HLOCAL  LocalMemHandle = 0;
#endif
   int col, row;
   jsebool MemGivenToClipboard;

   assert( This->Selected );

   for ( row = This->SelectedRect.top, col = This->SelectedRect.left;
         row <= This->SelectedRect.bottom  &&  row < This->NumLines;
         row++, col = 0 ) {
      jsebool AddCrLf = True;
      jsechar *line = This->Data[row];
      int RowLen;
      if ( NULL == line ) {
         RowLen = 0;
      } else {
         int EndCol;
         RowLen = strlen_jsechar(line);
         EndCol = ( row == This->SelectedRect.bottom ) ? This->SelectedRect.right : This->scrWidth ;
         if ( EndCol <= RowLen ) {
            AddCrLf = False;
            RowLen = EndCol;
         } /* endif */
         RowLen -= col;
         if ( RowLen <= 0 )
            RowLen = 0;
         else
            line += col;
      } /* endif */
      assert( 0 <= RowLen );

      // Windows CE will use LocalMemHandle and jsemainwindowAddToLocalClipMemory() 
      if ( RowLen )
         #if defined(__JSE_WINCE__)
            jsemainwindowAddToLocalClipMemory(line,RowLen,&LocalMemHandle,&TotalClipSize);
         #else
            jsemainwindowAddToGlobalClipMemory(line,RowLen,&GlobalMemHandle,&TotalClipSize);
         #endif
      if ( AddCrLf )
         #if defined(__JSE_WINCE__)
            jsemainwindowAddToLocalClipMemory(UNISTR("\r\n"),2,&LocalMemHandle,&TotalClipSize);
         #else
            jsemainwindowAddToGlobalClipMemory(UNISTR("\r\n"),2,&GlobalMemHandle,&TotalClipSize);
         #endif
   
   } /* endfor */

   MemGivenToClipboard = False;
   if ( OpenClipboard(This->hWindow) ) {
      EmptyClipboard();
#if defined(__JSE_WINCE__) // Windows CE will use LocalMemHandle
      if ( LocalMemHandle ) {
         if ( SetClipboardData(CF_TEXT,LocalMemHandle) )
            MemGivenToClipboard = True;
      } /* endif */
#else
      if ( GlobalMemHandle ) {
         if ( SetClipboardData(CF_TEXT,GlobalMemHandle) )
            MemGivenToClipboard = True;
      } /* endif */
#endif
      CloseClipboard();
   } /* endif */

#if defined(__JSE_WINCE__)
   //Windows CE will use LocalMemHandle and LocalFree
   if ( !MemGivenToClipboard  &&  LocalMemHandle )
      LocalFree(LocalMemHandle);
#else
   if ( !MemGivenToClipboard  &&  GlobalMemHandle )
      GlobalFree(GlobalMemHandle);
#endif
}

   static void NEAR_CALL
jsemainwindowPutCharInBuffer(struct jseMainWindow *This,int c,int repeat)
{
   while( 0 < repeat-- ) {
      if ( This->KeysInKeyboardBuffer < KEYBOARD_BUFFER_MAX ) {
         This->KeyboardBuffer[This->KeysInKeyboardBuffer++] = c;
      } /* endif */
   } /* endwhile */
}

   static void NEAR_CALL
jsemainwindowPaste(struct jseMainWindow *This)
{
   if ( OpenClipboard(This->hWindow) ) {
      HANDLE hClip = GetClipboardData(CF_TEXT);
#if defined(__JSE_WINCE__)
// Windows CE will use LocalLock/LocalUnlock (defined in winbase.h)
      if ( NULL != hClip ) {
         LPSTR data = (LPSTR)LocalLock(hClip);
         if ( NULL != data ) {
            DWORD size = LocalSize(hClip);
            while ( size--  &&  *data ) {
               jsemainwindowPutCharInBuffer(This,*(data++),1);
            } /* endwhile */
            LocalUnlock(hClip);
         } /* endif */
      } /* endif */
#else
      if ( NULL != hClip ) {
         LPSTR data = (LPSTR)GlobalLock(hClip);
         if ( NULL != data ) {
            DWORD size = GlobalSize(hClip);
            while ( size--  &&  *data ) {
               jsemainwindowPutCharInBuffer(This,*(data++),1);
            } /* endwhile */
            GlobalUnlock(hClip);
         } /* endif */
      } /* endif */
#endif
      CloseClipboard();
   } /* endif */
}

   void NEAR_CALL
jsemainwindowMouseCapture(struct jseMainWindow *This,jsebool set/*else release*/)
{
   if ( set ) {
      if ( !This->CapturedMouse ) {
         This->CapturedMouse = True;
         SetCapture(This->hWindow);
      }
   } else {
      if ( This->CapturedMouse ) {
         This->CapturedMouse = False;
         ReleaseCapture();
      }
   } /* endif */
}

   static void NEAR_CALL
jsemainwindowInvalidateBetweenSelectedPoints(struct jseMainWindow *This,POINT pt1,POINT pt2)
{
   POINT pt, end;
   if ( pt1.y < pt2.y  ||  (pt1.y == pt2.y  &&  pt1.x < pt2.x) ) {
      pt = pt1;
      end = pt2;
   } else {
      pt = pt2;
      end = pt1;
   } /* endif */
   while ( pt.y < end.y  ||  pt.x < end.x ) {
      /* mark point for redrawing */
      RECT rect;
      rect.bottom = (rect.top = (pt.y - This->nVscrollPos) * This->cyChar) + This->cyChar;
      rect.right = (rect.left = (pt.x - This->nHscrollPos) * This->cxChar) + This->cxChar;

      InvalidateRect(This->hWindow,&rect,True);
      if ( This->scrWidth < ++(pt.x) ) {
         pt.x = 0;
         pt.y++;
      }
   }
}

   static void NEAR_CALL
jsemainwindowNoSelectedText(struct jseMainWindow *This)
{
   jsemainwindowMouseCapture(This,False);
   if ( This->Selected ) {
      POINT start, end;
      start.x = This->SelectedRect.left;  start.y = This->SelectedRect.top;
      end.x = This->SelectedRect.right;   end.y = This->SelectedRect.bottom;
      jsemainwindowInvalidateBetweenSelectedPoints(This,start,end);
      This->Selected = False;
   } /* endif */
}

   static void NEAR_CALL
jsemainwindowWmKeyDown(struct jseMainWindow *This,WPARAM wParam)
{
   /* check special ctrl-break key */
   if ( wParam == VK_CANCEL ) {
      qsignalSetBreak(&(((struct CEnviGlobal *)(This->appData))->GlobalSignal));

      /* check if this is special key-combination to paste */
   } else if ( (wParam == 'V'  &&  IS_KEY_DOWN(VK_CONTROL))
     || (wParam == VK_INSERT &&  IS_KEY_DOWN(VK_SHIFT)) ) {
      jsemainwindowPaste(This);

      /* copy marked text? */
   } else if ( This->Selected  &&  IS_KEY_DOWN(VK_CONTROL)
            && (wParam == 'C' || wParam == 'X' || wParam == VK_INSERT) ) {
      jsemainwindowCopySelectedTextToClipboard(This);
      if ( wParam == 'X' )
         jsemainwindowNoSelectedText(This);

      /* If these are function keys or direction (navigation) then add them
       * to the list of keys returned as if this were a WM_CHAR message
       */
   } else if ( (VK_PRIOR <= wParam && wParam <= VK_DELETE)
            || (VK_F1 <= wParam && wParam <= VK_F16) ) {
      jsemainwindowPutCharInBuffer(This,wParam | 0x100,1);
   }
}

   static jsebool NEAR_CALL
jsemainwindowWmChar(struct jseMainWindow *This,WPARAM wParam,LONG lParam)
{
   if ( 0 != wParam ) {
      if ( 0x03==wParam /* ctrl-C */  ||  0x18==wParam /* ctrl-X */ )
         return True;
      jsemainwindowNoSelectedText(This);
      if ( (VK_TAB == wParam)  &&  IS_KEY_DOWN(VK_SHIFT) )
         wParam |= 0x100;
      jsemainwindowPutCharInBuffer( This, wParam, LOWORD(lParam) );
      return(True);
   } /* endif */
   return(False);
}

   static void NEAR_CALL
jsemainwindowWmGetMinMaxInfo(struct jseMainWindow *This,LONG lParam)
{
   MINMAXINFO * info = (MINMAXINFO _FAR_ *)lParam;
   RECT rect;
   GetWindowRect(This->hWindow,&rect);

   info->ptMinTrackSize.x = (rect.right-rect.left+1)-This->cxClient+
     MIN_X_SIZE*This->cxChar;
   info->ptMinTrackSize.y = (rect.bottom-rect.top+1)-This->cyClient+
     MIN_Y_SIZE*This->cyChar;
}

   static void NEAR_CALL
jsemainwindowWmDropFiles(struct jseMainWindow *This,HDROP hDrop)
   {
#if !defined(__JSE_WINCE__)
   UINT FileCount, i, j;
   jsebool FirstName;
   jsechar FileNameBuf[(_MAX_PATH*2)+1];

   FileCount = DragQueryFile(hDrop,(UINT)-1,FileNameBuf,sizeof(FileNameBuf)/sizeof(FileNameBuf[0])-1);
   FirstName = True;
   for ( i = 0; i < FileCount; i++ ) {
      UINT NameLen = DragQueryFile(hDrop,i,FileNameBuf,sizeof(FileNameBuf)/sizeof(FileNameBuf[0])-1);
      if ( NameLen ) {
         if ( FirstName )
            FirstName = False;
         else
            jsemainwindowPutCharInBuffer(This,' ',1);
      } /* endif */
      for ( j = 0; j < NameLen; j++ )
         jsemainwindowPutCharInBuffer(This,FileNameBuf[j],1);
   } /* endfor */
   DragFinish(hDrop);
#endif /* !(__JSE_WINCE__) */
}

#define ConvertPixelColToSelectCol(PIXCOL)   ((PIXCOL)/This->cxChar) + This->nHscrollPos
#define ConvertPixelRowToSelectRow(PIXROW)   ((PIXROW - This->tmInternalLeading + This->cyChar/2 - 1)/This->cyChar) + This->nVscrollPos

   static void NEAR_CALL
jsemainwindowCapturedMouseMove(struct jseMainWindow *This,sint col,sint row)
{
   int OldnVscrollPos;
   int OldnHscrollPos;
   POINT SelectPoint;
   
   assert( This->CapturedMouse );
   assert( This->Selecting );
   /* in case a timer message is coming, save column and row */
   This->SelectTextTimerCol = col; This->SelectTextTimerRow = row;
   /* if coordinates outside of window then scroll */
   OldnVscrollPos = This->nVscrollPos;
   OldnHscrollPos = This->nHscrollPos;
   if ( row < 0 )
      SendMessage(This->hWindow,WM_VSCROLL,SB_LINEUP,0);
   else if ( This->cyClient <= row )
      SendMessage(This->hWindow,WM_VSCROLL,SB_LINEDOWN,0);
   if ( col < 0 )
      SendMessage(This->hWindow,WM_HSCROLL,SB_LINEUP,0);
   else if ( This->cxClient <= col )
      SendMessage(This->hWindow,WM_HSCROLL,SB_LINEDOWN,0);
   if ( (This->nVscrollPos != OldnVscrollPos) ||  (This->nHscrollPos != OldnHscrollPos) ) {
      /* setup timer to scroll screen again soon; if it fails then that's OK, just won't scroll automatically */
      SetTimer(This->hWindow,SELECT_TEXT_TIMER_ID,SELECT_TEXT_INTERVAL,NULL);
      /* adjust col and row for scrolled window */
      row += ( This->nVscrollPos - OldnVscrollPos ) * This->cyChar;
      col += ( This->nHscrollPos - OldnHscrollPos ) * This->cxChar;
   } /* endif */
   /* adjust row and col to fit on the screen */
   if ( col < 0 ) col = 0;     if ( This->cxClient <= col ) col = This->cxClient;
   if ( row < 0 ) row = 0;    if ( This->cyClient <= row ) row = This->cyClient;
   /* adjust row and col for character representation, not pixel */
   SelectPoint.y = ConvertPixelRowToSelectRow(row);
   SelectPoint.x = ConvertPixelColToSelectCol(col);
   /* set selected fields */
   if ( SelectPoint.y < This->SelectingStart.y
     || (SelectPoint.y == This->SelectingStart.y  &&  SelectPoint.x < This->SelectingStart.x) ) {
      /* new text comes before starting text */
      This->SelectedRect.top = SelectPoint.y;         This->SelectedRect.left = SelectPoint.x;
      This->SelectedRect.bottom = This->SelectingStart.y;   This->SelectedRect.right = This->SelectingStart.x;
   } else {
      This->SelectedRect.top = This->SelectingStart.y;      This->SelectedRect.left = This->SelectingStart.x;
      This->SelectedRect.bottom = SelectPoint.y;      This->SelectedRect.right = SelectPoint.x;
   }
   This->Selected = True;
   jsemainwindowInvalidateBetweenSelectedPoints(This,This->SelectingRecent,SelectPoint);
   This->SelectingRecent = SelectPoint;
}

   static void NEAR_CALL
jsemainwindowWmLButton(struct jseMainWindow *This,jsebool Down/*else up*/,sint col,sint row)
{
   if ( Down ) {
      #if defined(__NPEXE__)
         /* we don't get focus by default, so give it to ourselves */
         if ( This->hWindow != GetFocus()  ) {
            SetFocus( This->hWindow );
         } /* endif */
      #endif
      /* mouse pressed down */
      jsemainwindowNoSelectedText(This);  /* remove any reference to selected text if any */
      jsemainwindowMouseCapture(This,True);    /* in case not already capturing, make it start */
      This->Selecting = True;  /* start selecting text */
      This->SelectingStart.y = ConvertPixelRowToSelectRow(row);
      This->SelectingStart.x = ConvertPixelColToSelectCol(col);
      This->SelectingRecent = This->SelectingStart;
   } else {
      /* mouse released */
      if ( This->CapturedMouse ) {
         jsemainwindowCapturedMouseMove(This,col,row);  /* in case capturing, get this final position */
         assert( This->Selecting );
         jsemainwindowMouseCapture(This,False);   /* in case capturing mouse, stop capturing it */
         This->Selecting = False;  /* no longer selecting text */
      }
   } /* endif */
}

   static void NEAR_CALL
jsemainwindowSelectTextTimer(struct jseMainWindow *This)
{
   if ( This->CapturedMouse )
      jsemainwindowCapturedMouseMove(This,This->SelectTextTimerCol,This->SelectTextTimerRow);
}

   void NEAR_CALL
jsemainwindowSetVertScroll(struct jseMainWindow *This,jsebool RedrawPosition)
{
   This->nVscrollMax = max(0,This->NumLines - This->scrHeight );
   This->nVscrollPos = min(This->nVscrollPos,This->nVscrollMax);

#if 0
/* Only works if I have the Win 95 headers */

   SCROLLINFO si;
   si.fMask = SIF_ALL;
   si.cbSize = sizeof(si);
   si.nMin = 0;
   si.nMax = nVscrollMax;
   si.nPage = This->scrHeight;
   si.nPos = nVscrollPos;

   SetScrollInfo(This->hWindow,SB_VERT,&si,RedrawPosition);
#else
   SetScrollRange(This->hWindow,SB_VERT,0,This->nVscrollMax,FALSE);
   if ( RedrawPosition ) {
      SetScrollPos(This->hWindow,SB_VERT,This->nVscrollPos,TRUE);
   } /* endif */
#endif
}

#if 0
   void NEAR_CALL
jseMainWindow::SetHorzScroll()
{
   /*NumColumns = cxClient / This->cxChar ;*/
   nHscrollMax = max(0,This->scrWidth - cxClient / This->cxChar);
   nHscrollPos = min(nHscrollPos,nHscrollMax);

   SetScrollRange(This->hWindow,SB_HORZ,0,nHscrollMax,FALSE);
   SetScrollPos(This->hWindow,SB_HORZ,nHscrollPos,TRUE);
}
#endif

   short NEAR_CALL
jsemainwindowGetMaxWinWidth(struct jseMainWindow *This)
{
   return (short)( (This->scrWidth * This->cxChar) + (2 * GetSystemMetrics(SM_CXFRAME))
                 + GetSystemMetrics(SM_CXVSCROLL) );
}

   short NEAR_CALL
jsemainwindowGetMaxWinHeight(struct jseMainWindow *This)
{
   return (short)( (This->scrHeight * This->cyChar) + (2 * GetSystemMetrics(SM_CYFRAME))
                 + GetSystemMetrics(SM_CYCAPTION)
                 + GetSystemMetrics(SM_CYHSCROLL) );
}

   static void NEAR_CALL
jsemainwindowMoveToNextRow(struct jseMainWindow *This)  /* Increment This->CursorRow, move stuff around and scroll up screen if needed */
{
   assert( NULL != This->Data );
   assert( This->CursorRow < This->NumLines );
   assert( This->NumLines <= This->MaxNumLines );
   if ( ++(This->CursorRow) == This->NumLines ) {
      /* need to move on to the next line, so if can add more This->NumLines then do so */
      if ( This->NumLines < This->MaxNumLines ) {
         /* can add another line */
         This->NumLines++;
         This->Data = jseMustReMalloc(jsechar *,This->Data,This->NumLines * sizeof(This->Data[0]));
         /* reset the scroll bar for up to one more line */
         jsemainwindowSetVertScroll(This,False);
      } else {
         /* cannot add any more lines, and so move all lines up one row */
         assert( This->NumLines == This->MaxNumLines );
         This->CursorRow--;
         FreeIfNotNull(This->Data[0]);
         memmove(This->Data,&(This->Data[1]),(This->NumLines-1)*sizeof(This->Data[0]));
         if ( This->nVscrollMax <= This->nVscrollPos )
            This->nVscrollPos = This->nVscrollMax - 1;
         if ( This->Selected ) {
            if ( --This->SelectedRect.top < 0 ) {
               This->SelectedRect.top = 0;
               This->SelectedRect.left = 0;
            } /* endif */
            if ( --This->SelectedRect.bottom < 0 )
               jsemainwindowNoSelectedText(This);
         } /* endif */
      } /* endif */
      assert( This->CursorRow == (This->NumLines - 1) );
      This->Data[This->CursorRow] = NULL;
     #if defined(__OPEN32__)
/*      LPINT lpMaxPos, lpMinPos; */
/*      BOOL err = GetScrollRange(This->hWindow, SB_VERT, lpMinPos, lpMaxPos ); */
      int pos = GetScrollPos( This->hWindow, SB_VERT );
      int rc =  SetScrollPos( This->hWindow, SB_VERT, 100, TRUE );
     #else
      SendMessage(This->hWindow,WM_VSCROLL,SB_BOTTOM,0);
     #endif
   } /* endif */
}

   static void NEAR_CALL
jsemainwindowEndThisLine(struct jseMainWindow *This)
{
   /* end of this data line; save memory be remalloc'ing */
   jsechar *buf = This->Data[This->CursorRow];
   if ( NULL != buf ) {
      int buflen = strlen_jsechar(buf);
      if ( 0 == buflen ) {
         jseMustFree(buf);
         This->Data[This->CursorRow] = NULL;
      } else {
         This->Data[This->CursorRow] = jseMustReMalloc(jsechar,buf,sizeof(jsechar)*(buflen+1));
      } /* endif */
   } /* endif */
}

   static void NEAR_CALL
jsemainwindowMarkCharacterToBeRedisplayed(struct jseMainWindow *This,int CharCount)
{
   RECT rect;
   rect.bottom = (rect.top = (short)((This->CursorRow - This->nVscrollPos) * This->cyChar)) + This->cyChar;
   rect.right = (rect.left = (short)((This->CursorCol - This->nHscrollPos) * This->cxChar)) + (CharCount * This->cxChar);
   InvalidateRect(This->hWindow,&rect,True);
}

   static void NEAR_CALL
jsemainwindowDisplayWindowIfNotDisplayedAlready(struct jseMainWindow *This)
{
   assert( NULL != This->Data );
   if ( !This->DisplayedAlready ) {
      This->DisplayedAlready = True;
      ShowWindow(This->hWindow,SW_RESTORE);
      UpdateWindow(This->hWindow);
   }
}

   jsebool NEAR_CALL
jsemainwindowWriteToWindow(struct jseMainWindow *This,const jsechar *buffer)
{
   jsechar *buf;
   int OldLen, SrcLen, SpacesRemainingOnLine, PrintableCharLen;
   
   if ( This->AlreadyWritingToStdOut )
      return False;

   jsemainwindowDisplayWindowIfNotDisplayedAlready(This);
   This->AlreadyWritingToStdOut = True;
   if ( !This->ScrolledToBottom )
      SendMessage(This->hWindow,WM_VSCROLL,SB_BOTTOM,0);
   assert( This->CursorRow < This->NumLines );
   assert( This->CursorCol < This->scrWidth );
   NextBufferLine:
   /* Put this new data at This->CursorCol position in This->CursorRow */
   #define TABSIZE   8
   assert( This->CursorRow < This->NumLines );
   assert( This->CursorCol < This->scrWidth );
   assert( NULL != This->Data );
   buf = This->Data[This->CursorRow];
   OldLen = ( NULL == buf ) ? 0 : strlen_jsechar(buf);
   if ( OldLen < This->scrWidth )
   {
      buf = This->Data[This->CursorRow] = jseMustReMalloc(jsechar,This->Data[This->CursorRow],
               sizeof(jsechar)*(This->scrWidth+1));
      memset(buf+OldLen,0,sizeof(jsechar)*(This->scrWidth - OldLen + 1));
      /* if cursor is beyond string length then add spaces up to cursor */
      if ( OldLen < This->CursorCol )
      {
         int i = This->CursorCol-OldLen;
         while ( i-- )
            buf[OldLen+i] = ' ';
      } /* endif */
   } /* endif */
   /* copy data from source into the destination buffer */
   SrcLen = strlen_jsechar(buffer);
   while ( 0 != SrcLen ) {
      jsebool NewLine = False;
      assert( 0 < SrcLen );
      assert( This->CursorCol < This->scrWidth );
      /* treat tabs and such characters */
      /* for as long as possible in this line, just copy plain text */
      if ( 0 == (SpacesRemainingOnLine = This->scrWidth - This->CursorCol) ) {
         NewLine = True;
      } else {
         assert( 0 < SpacesRemainingOnLine );
         if ( 0 == (PrintableCharLen = strcspn_jsechar(buffer,UnprintableChars)) ) {
            /* special character */
            switch ( *buffer ) {
               case '\t':
               {  int CharsNeeded = TABSIZE - (This->CursorCol % TABSIZE) - 1;
                  {
                     int i = CharsNeeded+1;
                     while ( i-- )
                        buf[This->CursorCol+i] = ' ';
                  }
                  jsemainwindowMarkCharacterToBeRedisplayed(This,CharsNeeded);
                  This->CursorCol += CharsNeeded + 1;
               }  break;
               case '\n':
                  NewLine = True;
                  break;
               case '\r':
                  This->CursorCol = 0;
                  break;
               case '\010': /* backspace */
                  if ( This->CursorCol != 0 ) {
                     buf[--(This->CursorCol)] = ' ';
                     jsemainwindowMarkCharacterToBeRedisplayed(This,1);
                  } else {
                     assert( 0 == This->CursorCol );
                     jsemainwindowEndThisLine(This);
                     if ( 0 != This->CursorRow ) {
                        --This->CursorRow;
                        This->CursorCol = This->scrWidth - 1;
                        if ( NULL != This->Data[This->CursorRow]  &&  This->CursorCol < (int)strlen_jsechar(This->Data[This->CursorRow]) ) {
                           This->Data[This->CursorRow][This->CursorCol] = ' ';
                           jsemainwindowMarkCharacterToBeRedisplayed(This,1);
                        } /* endif */
                        goto NextBufferLine;
                     } /* endif */
                  } /* endif */
                  break;
               case '\a':  /* bell */
                  MessageBeep(0);
                  break;
               default:
                  break;
            } /* endswitch */
            buffer++;
            SrcLen--;
         } else {
            int CopyLen;
            /* standard character copied straight to buffer */
            assert( 0 < PrintableCharLen );
            CopyLen = min(SrcLen,min(SpacesRemainingOnLine,PrintableCharLen));
            assert( 0 < CopyLen );
            memcpy(buf+This->CursorCol,buffer,sizeof(jsechar)*CopyLen);
            jsemainwindowMarkCharacterToBeRedisplayed(This,CopyLen);
            This->CursorCol += CopyLen;
            buffer += CopyLen;
            SrcLen -= CopyLen;
         } /* endif */
      } /* endif */
      if ( NewLine  ||  This->scrWidth <= This->CursorCol ) {
         jsemainwindowEndThisLine(This);
         jsemainwindowMoveToNextRow(This);
         This->CursorCol = 0;
         goto NextBufferLine;
      } /* endif */
   } /* endwhile */
   if ( This->scrWidth <= This->CursorCol ) {
      jsemainwindowEndThisLine(This);
      jsemainwindowMoveToNextRow(This);
      This->CursorCol = 0;
   } /* endif */
   This->AlreadyWritingToStdOut = False;

   return True;
}

   void NEAR_CALL
jsemainwindowDetermineCharacterSize(struct jseMainWindow *This)
{
#if !defined(__JSE_WINCE__)
   TEXTMETRIC tm;
   HWND DeskWindow;
   HDC hdc = GetDC(DeskWindow = GetDesktopWindow());
   
   SelectObject(hdc,This->hFont ? This->hFont : GetStockObject(SYSTEM_FIXED_FONT));
   GetTextMetrics(hdc,&tm);
   This->cxChar = tm.tmAveCharWidth;
   This->cyChar = tm.tmHeight - (This->tmInternalLeading = tm.tmInternalLeading);
   ReleaseDC(DeskWindow,hdc);
#endif
}

   static void NEAR_CALL
jsemainwindowGetInitialSizes(struct jseMainWindow *This)
{
   jsemainwindowGetInitialConsoleFont(This);
   jsemainwindowDetermineCharacterSize(This);
   This->scrWidth = DEFAULT_SCREEN_WIDTH;
   This->scrHeight = DEFAULT_SCREEN_HEIGHT;
}

   void
jsemainwindowWinSetScreenSize(struct jseMainWindow *This,uint width,uint height)
{
   RECT rect,crect;
   
   jsemainwindowDisplayWindowIfNotDisplayedAlready(This);

   if ( This->MaxNumLines < This->scrHeight )
      This->MaxNumLines = This->scrHeight;
   GetWindowRect(This->hWindow,&rect);
   GetClientRect(This->hWindow,&crect);

   This->scrWidth = max(1,width);
   This->scrHeight = max(1,height);

   MoveWindow(This->hWindow,rect.left,rect.top,
              (rect.right-rect.left)-(crect.right)+
              This->scrWidth*This->cxChar,
              (rect.bottom-rect.top)-(crect.bottom)+
              This->scrHeight*This->cyChar,
              TRUE);

   /* move cursor within window */
   if ( This->NumLines <= This->CursorRow )
      This->CursorRow = This->NumLines - 1;
   if ( This->scrWidth <= This->CursorCol )
      This->CursorCol = This->scrWidth - 1;

   /* Whenever the screen changes size, snap to the bottom */

   /* first thing is sync up the scrolling based on new screen size */
   jsemainwindowSetVertScroll(This,True);

   SendMessage(This->hWindow,WM_VSCROLL,SB_BOTTOM,0);
}

#if !defined(__CENVI__)
   void NEAR_CALL
jseMainWindow::GetInitialConsoleFont()
{
   Foreground = GetSysColor(COLOR_WINDOWTEXT);
}
#endif   /* !defined(__CENVI__) */

#if !defined(__CENVI__)
   void NEAR_CALL
jsemainwindowGetInitialConsoleColor(struct jseMainWindow *This)
{
   This->Background = GetSysColor(COLOR_WINDOW);
   hBrush = CreateSolidBrush(Background);
   if ( NULL != hBrush ) {
      #if defined(__JSE_WIN16__)
         SetClassWord(This->hWindow,GCW_HBRBACKGROUND,(WORD)hBrush);
      #else
         SetClassLong(This->hWindow,GCL_HBRBACKGROUND,(LONG)hBrush);
      #endif
   } /* endif */
}
#endif   /* !defined(__CENVI__) */

   void
jsemainwindowWinSetRowMemory(struct jseMainWindow *This,uint RowsRemembered)
{
   if ( NULL != This->Data ) {
      This->MaxNumLines = max((int)RowsRemembered,This->scrHeight);
      while ( This->MaxNumLines <= This->NumLines ) {
         memcpy(This->Data,This->Data+1,(--This->NumLines)*sizeof(This->Data[0]));
         This->Data = jseMustReMalloc(jsechar *,This->Data,This->NumLines * sizeof(This->Data[0]));
      } /* endwhile */
      if ( This->NumLines <= This->CursorRow  )
         This->CursorRow = This->NumLines - 1;
      /* may display invalid data, so start at the top and scroll to the bottom */
      This->nVscrollPos = 0;
      SendMessage(This->hWindow,WM_VSCROLL,SB_BOTTOM,0);
      InvalidateRect(This->hWindow,NULL,True);
   } /* endif */
}

   void
jsemainwindowWinGetScreenSize(struct jseMainWindow *This,uint *width,uint *height)
{
   *width = This->scrWidth;
   *height = This->scrHeight;
}

   void
jsemainwindowWinScreenClear(struct jseMainWindow *This)
{
   int i;
   
   jsemainwindowDisplayWindowIfNotDisplayedAlready(This);
   InvalidateRect(This->hWindow,NULL,True);
   /* clear screen, first remote ALL rows */
   for ( i = 1; i <= This->NumLines; i++ ) {
      int row = This->NumLines - i;
      if ( row < 0 )
         break;
      FreeIfNotNull(This->Data[row]);
      This->Data[row] = NULL;
   } /* endfor */
   This->NumLines = 1;
   This->nVscrollMax = This->nVscrollPos = 0;
   jsemainwindowSetVertScroll(This,False);
   jsemainwindowWinSetCursorPosition(This,0,0);
}

   void
jsemainwindowWinGetCursorPosition(struct jseMainWindow *This,uint *col,uint *row)
{
   *col = This->CursorCol;
   *row = This->CursorRow - This->nVscrollPos;
}

   void
jsemainwindowWinSetCursorPosition(struct jseMainWindow *This,uint col,uint row)
{
   jsemainwindowDisplayWindowIfNotDisplayedAlready(This);
   assert( (int)col < This->scrWidth );
   assert( (int)row < This->scrHeight );
   jsemainwindowEndThisLine(This);
   /* make sure there are enough rows to put the cursor here */
   while ( This->NumLines <= (int)row ) {
      jsemainwindowMoveToNextRow(This);
   } /* endwhile */
   if ( !This->ScrolledToBottom )
      SendMessage(This->hWindow,WM_VSCROLL,SB_BOTTOM,0);
   This->CursorRow = row + This->nVscrollPos;
   while( This->NumLines <= This->CursorRow ) {
      This->CursorRow--;
      row--;
   } /* endwhile */
   assert( 0 <= This->CursorRow );
   assert( 0 <= (sint)row );
   This->CursorCol = col;
   if ( This->hWindow == GetFocus() ) {
      SetCaretPos((This->CursorCol - This->nHscrollPos) * This->cxChar,row * This->cyChar);
   } /* endif */
}

#endif  /* !defined(__CGI__) */

   WINDOWS_CALLBACK_FUNCTION(long)
jsemainwindowMainWindowProcedure(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
   struct jseMainWindow *MainWin = (struct jseMainWindow *)GetWindowLong(hwnd,0);

   if ( NULL == MainWin ) {
      if ( WM_CREATE == message ) {
         CREATESTRUCT _FAR_ *lpcs = (CREATESTRUCT _FAR_ *)lParam;
         struct jseMainWindow *MainWin = (struct jseMainWindow *)(lpcs->lpCreateParams);
         MainWin->hWindow = hwnd;
         SetWindowLong(hwnd,0,(LONG)MainWin);

         #if defined(__CENVI__)
            jsemainwindowAddCEnviMenuItems(MainWin);
         #endif

         return(0);
      } /* endif */
      return DefWindowProc(hwnd,message,wParam,lParam);
   } /* endif */

   assert( NULL != MainWin );
   assert( hwnd == MainWin->hWindow );

   switch (message) {
     #if !defined(__CGI__)
      case WM_SIZE:
         jsemainwindowWmSize(MainWin,wParam);
         break;

      case WM_SETFOCUS:
         jsemainwindowWmSetFocus(MainWin);
         return 0;

      case WM_KILLFOCUS:
         jsemainwindowWmKillFocus(MainWin);
         return 0;

      case WM_VSCROLL:
         jsemainwindowWmVScroll(MainWin,wParam,lParam);
         return 0;

      case WM_HSCROLL:
         jsemainwindowWmHScroll(MainWin,wParam,lParam);
         return 0;

      case WM_PAINT:
         jsemainwindowWmPaint(MainWin);
         return 0;

      case WM_KEYDOWN:
         jsemainwindowWmKeyDown(MainWin,wParam);
         break;

      case WM_CHAR:
         if ( jsemainwindowWmChar(MainWin,wParam,lParam) )
            return 0;
         break;

      /* I know the (int)(short) looks funny, but it is necessary to avoid
       *  losing the 'negativeness' of numbers.
       */
      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
         jsemainwindowWmLButton(MainWin,WM_LBUTTONDOWN==message,(int)(short)LOWORD(lParam),
                            (int)(short)HIWORD(lParam));
         return 0;

      case WM_MOUSEMOVE:
         if ( MainWin->CapturedMouse )
            jsemainwindowCapturedMouseMove(MainWin,(int)(short)LOWORD(lParam),
                                       (int)(short)HIWORD(lParam));
         break;

      case WM_GETMINMAXINFO:
         jsemainwindowWmGetMinMaxInfo(MainWin,lParam);
         break;

      case WM_DROPFILES:
         jsemainwindowWmDropFiles(MainWin,(HDROP)wParam);
         return 0;

      case WM_TIMER:
         if ( SELECT_TEXT_TIMER_ID == wParam )
            jsemainwindowSelectTextTimer(MainWin);
         #if !defined(__NPEXE__)
         else if ( MARQUEE_TIMER_ID == wParam )
            ScrollMarquee(MainWin);
         #endif
         break;

     #endif

#     if defined(__CENVI__)
         case WM_SYSCOMMAND:
#           if !defined(__CGI__) && !defined(__NPEXE__)
               if ( jsemainwindowProcessSysCommand(MainWin,wParam) )
                  return 0;
#           endif
            break;
#        if defined(__JSE_WIN32__) && defined(JSE_WIN_SUBCLASSWINDOW)
            case WM_COPYDATA:
               return jsemainwindowWmCopyData(lParam);
#        endif
#        if defined(__NPEXE__)
            /*case WM_NCHITTEST:*/
               /*SetFocus(hwnd);*/
               /*return HTCLIENT;*/
#        endif
#     endif   /* defined(__CENVI__) */

     #if defined(__NPEXE__)
      case WM_DESTROY:
     #endif
      case WM_CLOSE:
         #if defined(__CENVI__)
            qsignalSetDeath(&(((struct CEnviGlobal *)(MainWin->appData))->GlobalSignal));
         #endif
         return 0;   /* don't allow close yet */

   } /* endswitch */

   return DefWindowProc(hwnd,message,wParam,lParam);
}

   struct jseMainWindow * NEAR_CALL
jsemainwindowNew(void _FAR_ *appData,HINSTANCE phInstance,jsechar *ClassName)
{
   int x, y, width, height;
   struct jseMainWindow *This;
   
   This = jseMustMalloc(struct jseMainWindow,sizeof(struct jseMainWindow));
   memset(This,0,sizeof(*This));
   assert( 0 == DisableMultitaskingDepth );
   assert( 0 == This->SubTitleDepth );
   #if !defined(__CGI__)
      assert( NULL == This->hFont );
      assert( NULL == This->hBrush );
      assert( 0 == This->KeysInKeyboardBuffer );
      assert( 0 == This->MyFGetCLookahead );
      assert( 0 == This->CursorCol );
      assert( 0 == This->CursorRow );
      assert( 0 == This->nVscrollPos );
      assert( 0 == This->nHscrollPos );
      assert( !This->ScrolledToBottom );
      assert( !This->DisplayedAlready );
      assert( !This->AlreadyWritingToStdOut );
      assert( !This->Selected );
      assert( !This->Selecting );
      assert( !This->CapturedMouse );
      assert( 0 == This->painted );
   #endif

   /* initialize values */
   This->appData = appData;
   This->hInstance = phInstance;

   #if defined(__ACTIVEX__)
      This->pActiveX = pActiveX;
   #endif

   assert( '\0' == This->jseClassName[0] );
#if defined(JSE_WIN_SUBCLASSWINDOW)
   assert( NULL == This->SubclassProcInstance );
   assert( NULL == This->SubclassedWindowList );
#endif /* #if defined(JSE_WIN_SUBCLASSWINDOW) */

   x=y=width=height=0;
   #if !defined(__CGI__)
      This->MaxNumLines = DEFAULT_MAX_LINES;
      This->NumLines = 1;
      *(This->Data = jseMustMalloc(jsechar *,sizeof(This->Data[0]))) = NULL;

      jsemainwindowGetInitialSizes(This);

      #if defined(__CENVI__)
         jsemainwindowGetInitialDimensions(This,&x,&y,&width,&height);
      #else
         x = y = CW_USEDEFAULT;
         width = GetMaxWinWidth(); height = GetMaxWinHeight();
      #endif

      This->cxWindow = width; This->cyWindow = height;

   #endif

   #if defined(__CENVI__)
      jsemainwindowCreateCEnviWindow(This,ClassName,x,y,width,height);
   #else
      CreateWindow(ClassName,          /* window class name */
                   CEnviDefaultTitle,  /* window caption */
                   WS_OVERLAPPEDWINDOW | WS_VSCROLL, /* windows style */
                   x, y, width,height,
                   NULL,               /* parent window handle */
                   NULL,               /* window menu handle */
                   phInstance,         /* program instance handle */
                   (LPSTR)this );      /* creation parameters; pass this class */
   #endif

   #if !defined(__CGI__)
      jsemainwindowGetInitialConsoleColor(This);
   #endif
   return This;
}

   void NEAR_CALL
jsemainwindowDelete(struct jseMainWindow *This)
{
   jsemainwindowUnRegisterDefaultChildWindowForever(This);
   jsemainwindowFreeDefaultProcInstanceForever(This);
   #if !defined(__CGI__)
      assert(NULL != This->Data);
   #endif
   assert( '\0' == This->jseClassName[0] );
   #if defined(JSE_WIN_SUBCLASSWINDOW)
      assert( NULL == This->SubclassProcInstance );
   #endif/* if defined(JSE_WIN_SUBCLASSWINDOW) */
   assert( 0 == This->SubTitleDepth );
   #if !defined(__CGI__)
   {
      int r;
      for ( r = 0; r < This->NumLines; r++ ) {
         FreeIfNotNull( This->Data[r] );
      } /* endfor */
      jseMustFree(This->Data);
      if ( NULL != This->hFont )
         DeleteObject(This->hFont);
      if ( NULL != This->hBrush ) {
         #if defined(__JSE_WIN16__)
            SetClassWord(This->hWindow,GCW_HBRBACKGROUND,(WORD)(GetStockObject(WHITE_BRUSH)));
         #else
            SetClassLong(This->hWindow,GCL_HBRBACKGROUND,(LONG)(GetStockObject(WHITE_BRUSH)));
         #endif
         DeleteObject(This->hBrush);
      } /* endif */
   }
   #endif
   jseMustFree(This);
}

#endif

#endif
