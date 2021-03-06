# include "cppmmap.h"
# include <windows.h>

// XXX:SSE do
// XXX:to much repeact code 
int cppsrc_mapget_include ( LIST_TYPE2_(uchar) header_list, struct uchar *files, int64_t *mod64 )
{
  HANDLE fd = INVALID_HANDLE_VALUE;
  HANDLE map = INVALID_HANDLE_VALUE;
  DWORD fsl;
  DWORD fsh;
  BOOL sig;
  UCHAR *pRawBuf = NULL;
  FILETIME ftw;

  fd = CreateFileW (  files->str, 
                      GENERIC_READ, /*source only read */
                      FILE_SHARE_READ,  
                      NULL,       
                      OPEN_EXISTING, 
                      FILE_ATTRIBUTE_READONLY,     
                      NULL );  
   
  if (fd == INVALID_HANDLE_VALUE)     
    return -1; 
  else if (mod64 != null) {
    sig = GetFileTime (fd, NULL, NULL, (LPFILETIME)& ftw);
    assert (sig != FALSE);
    * mod64 = * (int64_t *)& ftw;
  }
  if (mod64 != null)
    * mod64 = * (int64_t *)& ftw;
  if ((fsl = GetFileSize (fd, & fsh)) == 0) {
    CloseHandle (fd);
    return 0;
  } else if (ktrue) {
     map = CreateFileMappingW 
       ( fd, NULL, PAGE_READONLY, 
       fsh,      
       fsl,   
       NULL );     
     assert (map != INVALID_HANDLE_VALUE);
     pRawBuf = MapViewOfFile ( map, FILE_MAP_READ, 0, 0, 0); 
     assert (pRawBuf != NULL);

     {
       /* get text format */
       UCHAR *pRawSLL;
       INT fSize2;
       BOOL bUcs2 = FALSE;
       BOOL bUtf8 = FALSE;
       BOOL bReverse = FALSE;
       INT fUse = IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_UNICODE_MASK;
       /*check is exist utf8 bom?*/
       if (fsl >= 3
         && ( (pRawBuf[0] == 0xEF)
         && (pRawBuf[1] == 0xBB)
         && (pRawBuf[2] == 0xBF)) ) 
       {
         pRawSLL = & pRawBuf[3];
         fSize2 = fsl - 3;
         bUtf8 = TRUE;
       }
       /*check is exist ucs-2 bom?*/
       else if (fsl >= 2
         && ( (pRawBuf[0] == 0xFF)
         && (pRawBuf[1] == 0xFE)) ) 
       {
         pRawSLL = & pRawBuf[2];
         fSize2 = fsl - 2;
         bUcs2 = TRUE;
         bReverse = FALSE;
       }
       else if (fsl >= 2
         && ( (pRawBuf[0] == 0xFE)
         && (pRawBuf[1] == 0xFF)) ) 
       {
         pRawSLL = & pRawBuf[2];
         fSize2 = fsl - 2;
         bUcs2 = TRUE;
         bReverse = TRUE;
       }
       else 
       {
         pRawSLL = pRawBuf;
         fSize2 = fsl;
       }
       if (fSize2 <= 0)
       {
         achieve __cleanup;
       }

       if (bUcs2 == FALSE
         &&  (bUtf8 == FALSE)) 
       {
         IsTextUnicode (pRawSLL, fSize2, & fUse);
         if (fUse & IS_TEXT_UNICODE_UNICODE_MASK) 
           bUcs2 = TRUE;
         if (fUse & IS_TEXT_UNICODE_REVERSE_MASK)
           bReverse = TRUE;
       }
       // status cov 
       // @1:check wrap | null [fisrt check]
       // @2:check # + '"' + string +'"' 
# define WRAP_TYPE_WINDOWS 0 // 0x0D0A 
# define WRAP_TYPE_UNIX 1    // 0x0A
# define WRAP_TYPE_MAC 2     // 0x0D 
# define WRAP_TYPE_UNSET -1

       if (bUcs2 != FALSE) {
         if (bReverse == FALSE) {
           // unicode - L-endian
           WCHAR *pBuGL = (PVOID) pRawSLL;
           INT fSize3 = fSize2 >> 1;
           INT fpS =0;
           INT esWrapType = WRAP_TYPE_UNSET;
           WCHAR wChunk = 0;
           INT wChunkNums = -1;
           INT qBlockLeft = -1;
           WCHAR cc;

           // try get wrap type 
           while (fpS < fSize3) {
             cc = pBuGL[fpS];
             switch (pRawSLL[fpS]) {
             case 0x000D:
               if (++fpS < fSize3)
                 if (pBuGL[fpS] == 0x000A)
                   esWrapType = WRAP_TYPE_WINDOWS, wChunk = 0x000D, wChunkNums = 2;
                 else 
                   esWrapType = WRAP_TYPE_MAC, wChunk = 0x000D, wChunkNums = 1;
               else achieve __cleanup;
               achieve out3 ;
             case 0x000A:
               esWrapType = WRAP_TYPE_UNIX, wChunk = 0x000A, wChunkNums = 1;
               achieve out3 ;
             default :
               ++ fpS;
               break;
             }
           }
           // if esWrapType := WRAP_TYPE_UNSET then line := 1 
        out3:fpS = 0;
             // extract line wrap
  #   define IS_USELESS_CHAR(__p) ( ((__p) == _GC ('\t')) || ((__p) == _GC (' ')))
  #   define IS_WRAP(__p)((__p) == wChunk)
  #   define EXTRACT_LINE_WRAP()                 \
             do {                              \
               do {                            \
                 if ((cc=pBuGL[fpS]) != wChunk)   \
                   break ;                     \
                 else                          \
                   fpS += wChunkNums;          \
               } while (fpS < fSize3);         \
               if (fpS >= fSize3)              \
                 achieve __cleanup;            \
               else ;                          \
             } while (0)   
  #   define EXTRACT_LINE_WRAP2()                 \
             do {                              \
               while (++fpS < fSize3)    {  \
                 if ((cc=pBuGL[fpS]) != wChunk)   \
                   break ;                     \
                 else                          \
                   fpS += wChunkNums;          \
               }                              \
               if (fpS >= fSize3)              \
                 achieve __cleanup;            \
               else ;                          \
             } while (0)   
  #   define SERACH_NEXT_LINE()                 \
             do {                              \
               do {                            \
                 if ((cc=pBuGL[fpS]) != wChunk)   \
                   continue ;                     \
                 else {                         \
                   fpS += wChunkNums;          \
                   break;                      \
                 }                             \
               } while (++fpS < fSize3);         \
               if (fpS >= fSize3)              \
                 achieve __cleanup;            \
               else ;                          \
             } while (0)              
           EXTRACT_LINE_WRAP ();

           do {
             // skip space or \t 
         Q4: if (IS_USELESS_CHAR (cc)) // XXX: _mm_cmpestrc do 
               continue ;
             else if (IS_WRAP (cc)) {
                // newline, reset status 
               EXTRACT_LINE_WRAP2 ();
               achieve Q4;
             } else {
               do {
                 if (IS_USELESS_CHAR (cc)) // XXX: _mm_cmpestrc do 
                   continue ;
                 else if (IS_WRAP (cc)) {
                   // newline, reset status 
                   EXTRACT_LINE_WRAP2 ();
                   achieve Q4;
                 } else if (cc != _GC ('#')) {
                   SERACH_NEXT_LINE();
                   EXTRACT_LINE_WRAP();
                   achieve Q4;
                 } else {
                    // match include
                   while (++ fpS < fSize3) {
                     cc = pBuGL[fpS];
                     if (IS_USELESS_CHAR (cc))
                       continue ;
                     else if (IS_WRAP (cc)) {
                       // newline, reset status 
                       EXTRACT_LINE_WRAP2 ();
                       achieve Q4;
                     } else if ( (fpS+10) >= fSize3) {
                       achieve __cleanup;// no size to read,
                     } else {
                       // try match include 
                       if ( (pBuGL[fpS+0] == _GC('i'))
                         && (pBuGL[fpS+1] == _GC('n'))
                         && (pBuGL[fpS+2] == _GC('c'))
                         && (pBuGL[fpS+3] == _GC('l'))
                         && (pBuGL[fpS+4] == _GC('u'))
                         && (pBuGL[fpS+5] == _GC('d'))
                         && (pBuGL[fpS+6] == _GC('e')) )
                       {
                         // try match left " 
                         fpS += 6;
                         qBlockLeft = -1;
                         while (++ fpS < fSize3) {
                           cc = pBuGL[fpS];
                           if (IS_USELESS_CHAR (cc))
                             continue ;
                           else if (IS_WRAP (cc)) {
                             // newline, reset status 
                             EXTRACT_LINE_WRAP2 ();
                             achieve Q4;
                           } else if (cc == _GC ('"')) {
                             if (qBlockLeft == -1)
                               qBlockLeft = fpS; 
                             else {
                               // calc path... TODO: clear space and more check 
                               struct uchar *pt;
                               uchar_init (& pt);
                               uchar_assign (pt, & pBuGL[qBlockLeft+1], fpS - qBlockLeft - 1);
                               list_insert_tail (header_list, pt);
                               SERACH_NEXT_LINE();
                               EXTRACT_LINE_WRAP();
                               achieve Q4; 
                             }
                           } else if (qBlockLeft == -1) {
                              SERACH_NEXT_LINE();
                              EXTRACT_LINE_WRAP();
                              achieve Q4; 
                           }
                         }
                         achieve __cleanup;// no size to read,
                       }
                       SERACH_NEXT_LINE();
                       EXTRACT_LINE_WRAP();
                       achieve Q4; 
                     }
                   }
                   achieve __cleanup;// no size to read,
                 }
               } while (++ fpS < fSize3);

             }
           } while (++ fpS < fSize3);
         } else {
           // unicode - B-endian
           WCHAR *pBuGL = (PVOID) pRawSLL;
           INT fSize3 = fSize2 >> 1;
           INT fpS =0;
           INT esWrapType = WRAP_TYPE_UNSET;
           WCHAR wChunk = 0;
           INT wChunkNums = -1;
           INT qBlockLeft = -1;
           WCHAR cc;
           WCHAR space = _GC (' ');
           WCHAR tab  = _GC ('\t');
           WCHAR st = _GC ('#');
           WCHAR st2 = _GC ('"');
           WCHAR ct = _GC ('i');
           WCHAR ct2 = _GC ('n');      
           WCHAR ct3 = _GC ('c');
           WCHAR ct4 = _GC ('l');
           WCHAR ct5 = _GC ('u');      
           WCHAR ct6 = _GC ('d');
           WCHAR ct7 = _GC ('e');
         
           space = (((uint16_t)(space)) >> 8) | ((((uint16_t)(space)) & 0xFF) << 8);
           tab = (((uint16_t)(tab)) >> 8) | ((((uint16_t)(tab)) & 0xFF) << 8);  
           st = (((uint16_t)(st)) >> 8) | ((((uint16_t)(st)) & 0xFF) << 8); 
           st2 = (((uint16_t)(st2)) >> 8) | ((((uint16_t)(st2)) & 0xFF) << 8); 
           ct = (((uint16_t)(ct)) >> 8) | ((((uint16_t)(ct)) & 0xFF) << 8); 
           ct2 = (((uint16_t)(ct2)) >> 8) | ((((uint16_t)(ct2)) & 0xFF) << 8); 
           ct3 = (((uint16_t)(ct3)) >> 8) | ((((uint16_t)(ct3)) & 0xFF) << 8); 
           ct4 = (((uint16_t)(ct4)) >> 8) | ((((uint16_t)(ct4)) & 0xFF) << 8); 
           ct5 = (((uint16_t)(ct5)) >> 8) | ((((uint16_t)(ct5)) & 0xFF) << 8); 
           ct6 = (((uint16_t)(ct6)) >> 8) | ((((uint16_t)(ct6)) & 0xFF) << 8); 
           ct7 = (((uint16_t)(ct7)) >> 8) | ((((uint16_t)(ct7)) & 0xFF) << 8); 

           // try get wrap type 
           while (fpS < fSize3) {
             cc = pBuGL[fpS];
             switch (pBuGL[fpS]) {
             case 0x0D00:
               if (++fpS < fSize3)
                 if (pBuGL[fpS] == 0x0A00)
                   esWrapType = WRAP_TYPE_WINDOWS, wChunk = 0x0D00, wChunkNums = 2;
                 else 
                   esWrapType = WRAP_TYPE_MAC, wChunk = 0x0D00, wChunkNums = 1;
               else achieve __cleanup;
               achieve out37 ;
             case 0x0A00:
               esWrapType = WRAP_TYPE_UNIX, wChunk = 0x0A00, wChunkNums = 1;
               achieve out37 ;
             default :
               ++ fpS;
               break;
             }
           }
           // if esWrapType := WRAP_TYPE_UNSET then line := 1 
        out37:fpS = 0;
             // extract line wrap
  #   undef IS_USELESS_CHAR
  #   define IS_USELESS_CHAR(__p) ( ((__p) == tab) || ((__p) == space))
           EXTRACT_LINE_WRAP ();

           do {
             // skip space or \t 
         G4: if (IS_USELESS_CHAR (cc)) // XXX: _mm_cmpestrc do 
               continue ;
             else if (IS_WRAP (cc)) {
                // newline, reset status 
               EXTRACT_LINE_WRAP2 ();
               achieve G4;
             } else {
               do {
                 if (IS_USELESS_CHAR (cc)) // XXX: _mm_cmpestrc do 
                   continue ;
                 else if (IS_WRAP (cc)) {
                   // newline, reset status 
                   EXTRACT_LINE_WRAP2 ();
                   achieve G4;
                 } else if (cc != st) {
                   SERACH_NEXT_LINE();
                   EXTRACT_LINE_WRAP();
                   achieve G4;
                 } else {
                    // match include
                   while (++ fpS < fSize3) {
                     cc = pBuGL[fpS];
                     if (IS_USELESS_CHAR (cc))
                       continue ;
                     else if (IS_WRAP (cc)) {
                       // newline, reset status 
                       EXTRACT_LINE_WRAP2 ();
                       achieve G4;
                     } else if ( (fpS+10) >= fSize3) {
                       achieve __cleanup;// no size to read,
                     } else {
                       // try match include 
                       if ( (pBuGL[fpS+0] == ct)
                         && (pBuGL[fpS+1] == ct2)
                         && (pBuGL[fpS+2] == ct3)
                         && (pBuGL[fpS+3] == ct4)
                         && (pBuGL[fpS+4] == ct5)
                         && (pBuGL[fpS+5] == ct6)
                         && (pBuGL[fpS+6] == ct7) )
                       {
                         // try match left " 
                         fpS += 6;
                         qBlockLeft = -1;
                         while (++ fpS < fSize3) {
                           cc = pBuGL[fpS];
                           if (IS_USELESS_CHAR (cc))
                             continue ;
                           else if (IS_WRAP (cc)) {
                             // newline, reset status 
                             EXTRACT_LINE_WRAP2 ();
                             achieve G4;
                           } else if (cc == st2) {
                             if (qBlockLeft == -1)
                               qBlockLeft = fpS; 
                             else {
                               // calc path... TODO: clear space and more check 
                               struct uchar *pt;
                               uchar_init (& pt);
                               // unicode B-endian to - L-endian 
                               {
                                 INT id; 
                                 INT id7 = fpS - qBlockLeft - 1;
                                 WCHAR *pQS = & pBuGL[qBlockLeft+1];
                                 uchar_expand (pt, id7+3);
                                 for (id = 0; id != id7; id++) {
                                    pt->str[id] = (((uint16_t)(pQS [id])) >> 8) | ((((uint16_t)(pQS [id])) & 0xFF) << 8);
                                 }
                                 uchar_setlens (pt, id7);
                               }
                               list_insert_tail (header_list, pt);
                               SERACH_NEXT_LINE();
                               EXTRACT_LINE_WRAP();
                               achieve G4; 
                             }
                           } else if (qBlockLeft == -1) {
                              SERACH_NEXT_LINE();
                              EXTRACT_LINE_WRAP();
                              achieve G4; 
                           }
                         }
                         achieve __cleanup;// no size to read,
                       }
                       SERACH_NEXT_LINE();
                       EXTRACT_LINE_WRAP();
                       achieve G4; 
                     }
                   }
                   achieve __cleanup;// no size to read,
                 }
               } while (++ fpS < fSize3);

             }
           } while (++ fpS < fSize3);
         }
       } else {
         // ascii | utf-8 
         UCHAR *pBuGL = pRawSLL;
         INT fSize3 = fSize2;
         INT fpS =0;
         INT esWrapType = WRAP_TYPE_UNSET;
         UCHAR wChunk = 0;
         INT wChunkNums = -1;
         INT qBlockLeft = -1;
         UCHAR cc;

         // try get wrap type 
         while (fpS < fSize3) {
           cc = pBuGL[fpS];
           switch (pRawSLL[fpS]) {
           case 0x0D:
             if (++fpS < fSize3)
               if (pBuGL[fpS] == 0x0A)
                 esWrapType = WRAP_TYPE_WINDOWS, wChunk = 0x0D, wChunkNums = 2;
               else 
                 esWrapType = WRAP_TYPE_MAC, wChunk = 0x0D, wChunkNums = 1;
             else achieve __cleanup;
             achieve out7 ;
           case 0x0A:
             esWrapType = WRAP_TYPE_UNIX, wChunk = 0x0A, wChunkNums = 1;
             achieve out7 ;
           default :
             ++ fpS;
             break;
           }
         }
         // if esWrapType := WRAP_TYPE_UNSET then line := 1 
      out7:fpS = 0;
           // extract line wrap
#   undef IS_USELESS_CHAR
#   define IS_USELESS_CHAR(__p) ( ((__p) == _QC ('\t')) || ((__p) == _QC (' ')) || ((__p) & 0x80))
         EXTRACT_LINE_WRAP ();

         do {
           // skip space or \t 
       L4: if (IS_USELESS_CHAR (cc)) // XXX: _mm_cmpestrc do 
             continue ;
           else if (IS_WRAP (cc)) {
              // newline, reset status 
             EXTRACT_LINE_WRAP2 ();
             achieve L4;
           } else {
             do {
               if (IS_USELESS_CHAR (cc)) // XXX: _mm_cmpestrc do 
                 continue ;
               else if (IS_WRAP (cc)) {
                 // newline, reset status 
                 EXTRACT_LINE_WRAP2 ();
                 achieve L4;
               } else if (cc != _QC ('#')) {
                 SERACH_NEXT_LINE();
                 EXTRACT_LINE_WRAP();
                 achieve L4;
               } else {
                  // match include
                 while (++ fpS < fSize3) {
                   cc = pBuGL[fpS];
                   if (IS_USELESS_CHAR (cc))
                     continue ;
                   else if (IS_WRAP (cc)) {
                     // newline, reset status 
                     EXTRACT_LINE_WRAP2 ();
                     achieve L4;
                   } else if ( (fpS+10) >= fSize3) {
                     achieve __cleanup;// no size to read,
                   } else {
                     // try match include 
                     if ( (pBuGL[fpS+0] == _QC('i'))
                       && (pBuGL[fpS+1] == _QC('n'))
                       && (pBuGL[fpS+2] == _QC('c'))
                       && (pBuGL[fpS+3] == _QC('l'))
                       && (pBuGL[fpS+4] == _QC('u'))
                       && (pBuGL[fpS+5] == _QC('d'))
                       && (pBuGL[fpS+6] == _QC('e')) )
                     {
                       // try match left " 
                       fpS += 6;
                       qBlockLeft = -1;
                       while (++ fpS < fSize3) {
                         cc = pBuGL[fpS];
                         if (IS_USELESS_CHAR (cc))
                           continue ;
                         else if (IS_WRAP (cc)) {
                           // newline, reset status 
                           EXTRACT_LINE_WRAP2 ();
                           achieve L4;
                         } else if (cc == _QC ('"')) {
                           if (qBlockLeft == -1)
                             qBlockLeft = fpS; 
                           else {
                             // calc path... TODO: clear space and more check, utf-8 MSB bit check .. 
                             struct uchar *pt;
                             uchar_init (& pt);
                             // uchar_assign (pt, & pBuGL[qBlockLeft+1], fpS - qBlockLeft - 1);
                             // utf8's ascii part to unicode- L-endian 
                             {
                               INT id; 
                               INT id7 = fpS - qBlockLeft - 1;
                               UCHAR *pQS = & pBuGL[qBlockLeft+1];
                               uchar_expand (pt, id7+3);
                               for (id = 0; id != id7; id++) {
                                  WCHAR umix = 0;
                                  umix = pQS [id];
                                  pt->str[id] = umix;
                               }
                               uchar_setlens (pt, id7);
                             }
                             
                             list_insert_tail (header_list, pt);
                             SERACH_NEXT_LINE();
                             EXTRACT_LINE_WRAP();
                             achieve L4; 
                           }
                         } else if (qBlockLeft == -1) {
                            SERACH_NEXT_LINE();
                            EXTRACT_LINE_WRAP();
                            achieve L4; 
                         }
                       }
                       achieve __cleanup;// no size to read,
                     }
                     SERACH_NEXT_LINE();
                     EXTRACT_LINE_WRAP();
                     achieve L4; 
                   }
                 }
                 achieve __cleanup;// no size to read,
               }
             } while (++ fpS < fSize3);

           }
         } while (++ fpS < fSize3);
       }
     }
__cleanup: 
     UnmapViewOfFile (pRawBuf);
     CloseHandle (map);
     CloseHandle (fd);
     return 0;
  }
  return 0;  
}


# if 0

int main(void)
{

   struct list_* sad;
   struct uchar *qa;

   uchar_ginit ();
   list_init (&sad);
   uchar_init (&qa);
   uchar_assign2(qa, _GR("E:\\umake\\ctl.c"));
   cppsrc_mapget_include (sad, qa);

   return 0;


}
# endif 