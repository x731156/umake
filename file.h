#if !defined (__UMAKE_FILE2_H_)
#  define __UMAKE_FILE2_H_ 

#include "config.h"
#include "list.h"
#include "uchar.h"
#include "path.h"

struct file_;

/* simple file op
 */
# define FILE2_EXIST 0
# define FILE2_EXIST_ORCREATE_CLEAR 1
//
# define FILE2_READ 1
# define FILE2_READ_WRITE 2

int file_open (struct file_ **file_, uconst us *fname, int open_type, int access);
void file_close (struct file_ **file_);
int file_flush (struct file_ *file_);
int file_read (struct file_ *file_, int pos, void *buf, int size, int *nums_opr);
int file_write (struct file_ *file_, int pos, void *buf, int size, int *nums_opw);
int file_write2 (struct file_ *file_, int pos, uconst us *format, ...);
int file_write3 (struct file_ *file_, int pos, const unsigned char *format, ...);
int file_exist (uconst us *name);
int file_lastmod9 (uconst us *name, int64_t *recv);
int file_setpos (struct file_ *file_, int pos);
int file_del (uconst us *name);
int file_lastmod (struct file_ *file_, uint64_t *rev);
int file_size (struct file_ *file_, int *rev);
// int file_map (struct file_ *file_, us *gsym_inos, int len);
// int file_map_close (struct file_ *file_);
// void *file_map_set (struct file_ *file_, int size);
int32_t crc32_get (int32_t init, void *buf, int nums);
void slashtail_clear (us *bufh, int lens, int *ofbl_noslash);
void slashclr_getpath (us *bufh, int lens, int *ofbl_noslash);

LIST_TYPE2_(uchar) set_serach (uconst us *path, uconst us *suffix_, kbool *harm);
int slash_calc3 (us *bufh, int lens, int *ofb_inpure, kbool *slash_intail);
// LIST_TYPE_(kpath) struct list_ *file_serach (uconst us *path, uconst us *suffix_, kbool *harmful, kbool dep_use);

#endif 