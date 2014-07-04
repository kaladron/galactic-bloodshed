/*
 * 	@(#)file.h	3.1	(ULTRIX)	4/20/90
 */


/************************************************************************
 *									*
 *			Copyright (c) 1985 - 1989 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/************************************************************************
 *			Modification History				*
 *									*
 *	David L Ballenger, 28-Mar-1985					*
 * 0001	Modify so that <fcntl.h> can simply include this file.  This	*
 *	file already contains all the definitions contained in		*
 *	<fcntl.h>, plus all the definitions that would need to be added	*
 *	to <fcntl.h> from the BRL version for System V emulation.  No	*
 *	sense in defining things in multiple places.			*
 *									*
 *	Larry Cohen, 4-April-1985					*
 * 0002 Changes to support open block if in use capability		*
 *	FBLKINUSE							*
 *	O_BLKINUSE - open blocks if IINUSE bit in inode is set		*
 *									*
 *	FBLKANDSET							*
 *	O_BLKANDSET - open blocks if IINUSE bit in inode is set		*
 *			and sets after open succeeds.			*
 *									*
 * 	Greg Depp 8-April-1985						*
 * 0003 Added DTYPE_PORT to define System V IPC Named Pipe type		*
 *									*
 *	Stephen Reilly, 09-Sept-85					*
 * 	Modified to handle the new lockf code.				*
 *									*
 *	Paul Shaughnessy, 24-December-1985				*
 * 0004 Added syncronous write capability in open and fcntl system      *
 *	calls.								*
 *									*
 *	11-Mar-86	Larry Palmer 					*
 *	Added flag to mark a file as being used by n-bufferring.        *
 *									*
 *	Tim Burke,	10-June-1986					*
 * 0005 Inserted FTERMIO and O_TERMIO which tells if the program is to	*
 *	run as a System V program.  If so use a "RAW" type of default   *
 *	settings rather that the normal "COOKED" style on terminal 	*
 *	line defaults.							*
 *									*
 *	Tim Burke,	1-Dec-1987					*
 * 0006 Inserted O_NONBLOCK which will be used to specify		*
 *	the POSIX no delay open and I/O.				*
 *									*
 *	Mark Parenti,	13-Jan-1988					*
 * 0007	Moved define of R_OK, etc to KERNEL only as they are		*
 *	defined in <unistd.h> for user level.  Add O_ACCMODE POSIX	*
 *	definition and change types of some structure entries.		*
 *									*
 * 	Mark Parenti, 	15-Jan-1988					*
 * 0008	Change definition of R_OK, etc. conditional on the previous	*
 *	declaration of R_OK, etc.  This allows existing programs	*
 *	which use file.h for the above definitions to continue to 	*
 *	compile.							*
 *	Paul Shaughnessy,	10-Feb-1988				*
 * 0009 Added a PIPE flag, and changed definition of POSIX NONBLOCK	*
 *	flag.								*
 *									*
 *	22-Aug-88	Paul Shaughnessy				*
 *	Added FCNTLONLYSET define to aid in cleaning			*
 *	up F_GETFL and F_SETFL fcntl(2) requests.			*
 *									*
 *	Jon Reeves	30-May-1989					*
 *	Added parens around O_ACCMODE definition; moved unruly comments *
 *	Changed protection variable name for ANSI compliance		*
 *									*
 *	Jon Reeves	11-Jul-1989					*
 *	Added function declarations for X/Open compliance		*
 *									*
 *	Paul Shaughnessy	17-Jan-1990				*
 *	Added _POSIX_SOURCE and _XOPEN_SOURCE test macros		*
 *									*
 ************************************************************************/

/*
 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 ! WARNING !!!!!!!! This header file is described in XOPEN. To add macros !
 ! or defines or structures, you must inclose them as follows:            !
 ! #if !defined(_POSIX_SOURCE)                                            !
 ! #endif                                                                 !
 ! Failure to do so, will break compatibility with the standard.          !
 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

/* Don't define things twice.  This protects from a source file which
 * includes both <fcntl.h> and <sys/file.h>
 */
#ifndef _FCNTL_H_
#define _FCNTL_H_

#if !defined(KERNEL)
#include <sys/types.h>
#endif

#ifdef KERNEL && !defined(_POSIX_SOURCE)
/*
 * Descriptor table entry.
 * One for each kernel object.
 */
struct	file {
	int	f_flag;		/* see below */
	short	f_type;		/* descriptor type */
	short	f_count;	/* reference count */
	short	f_msgcount;	/* references from message queue */
	struct	fileops {
/*		return	function	arguments		*/
		int	(*fo_rw)(	/* fp,uio,rw		*/ );
		int	(*fo_ioctl)(	/* fp,com,data,cred	*/ );
		int	(*fo_select)(	/* fp,which		*/ );
		int	(*fo_close)(	/* fp			*/ );
	} *f_ops;
	caddr_t	f_data;		/* inode */
	off_t	f_offset;
	struct ucred *f_cred;
	struct lock_t f_lk;
};

extern  struct lock_t lk_file;
struct	file *file, *fileNFILE;
int	nfile;
struct	file *getf();
struct	file *falloc();
#endif	/* KERNEL && NOT _POSIX_SOURCE */

/*
 * flags - also some values are used by fcntl(2)
 */
#define	_FOPEN			 (-1)
#define	_FREAD			00001		/* descriptor read/receive'able */
#define	_FWRITE			00002		/* descriptor write/send'able */
#define	_FNDELAY		00004		/* no delay */
#define	_FAPPEND		00010		/* append on each write */
#define	_FMARK			00020		/* mark during gc() */
#define	_FDEFER			00040		/* defer for next gc pass */
#define	_FASYNC			00100		/* signal pgrp when data ready */
#define	_FSHLOCK		00200		/* shared lock present */
#define	_FEXLOCK		00400		/* exclusive lock present */
#define	_FCREAT			01000		/* create if nonexistant */
#define	_FTRUNC			02000		/* truncate to zero length */
#define	_FEXCL			04000		/* error if already created */
#define _FBLKINUSE	       010000		/* block if "in use"	*0002*/
#define _FBLKANDSET	      (020000|_FBLKINUSE)/* block, test and set "in use" */
#define _FSYNCRON	      0100000		/* Write file syncronously *0004*/
#define _FNBUF		      0200000		/* file used for n-buffering */
#define _FNBLOCK	      0400000		/* POSIX no delay */
#define _FTERMIO	     01000000		/* termio style program */
#define _FNOCTTY	     02000000		/* termio no controlling tty */
#if	!defined(_POSIX_SOURCE)
#define	FOPEN		_FOPEN
#define	FREAD		_FREAD
#define	FWRITE		_FWRITE
#define	FNDELAY		_FNDELAY
#define	FAPPEND		_FAPPEND
#define	FMARK		_FMARK
#define	FDEFER		_FDEFER
#define	FASYNC		_FASYNC
#define	FSHLOCK		_FSHLOCK
#define	FEXLOCK		_FEXLOCK
#define	FCREAT		_FCREAT
#define	FTRUNC		_FTRUNC
#define	FEXCL		_FEXCL
#define FBLKINUSE	_FBLKINUSE
#define FBLKANDSET	_FBLKANDSET
#define FSYNCRON	_FSYNCRON
#define FNBUF		_FNBUF
#define FNBLOCK		_FNBLOCK
#define FTERMIO		_FTERMIO
#define FNOCTTY		_FNOCTTY

/* bits to save after open */
#define	FMASK		0510113     /* 0004 */
/*
 * FCNTLCANT are flags that are used by the kernel exclusively;
 * the user can't set them via fcntl(2).
 */
#define	FCNTLCANT	(FREAD|FWRITE|FMARK|FDEFER|FSHLOCK|FEXLOCK|FNBUF)
/*
 * FCNLTONLYSET are a set of flags that the user may set via a F_SETFL
 * fcntl(2) request.
 */
#define FCNTLONLYSET	(FNDELAY|FAPPEND|FASYNC|FSYNCRON|FNBLOCK)

#endif	/* NOT _POSIX_SOURCE */

/*
 * Base, POSIX, and XOPEN defines....
 *
 * File access modes for open() and fcntl()
 */
#define	O_RDONLY	000		/* open for reading */
#define	O_RDWR		002		/* open for read & write */
#define	O_WRONLY	001		/* open for writing */
/*
 * File status flags for open() and fcntl()
 */
#define	O_APPEND	_FAPPEND	/* append on each write */
#define O_NONBLOCK	_FNBLOCK	/* POSIX non-blocking I/O */
#if	!defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
#define O_SYNC		_FSYNCRON	/* system V synchronous write */
#define	O_NDELAY	_FNDELAY	/* non-blocking open */
#endif
/*
 * Values for oflag used by open()
 */
#define	O_CREAT		_FCREAT		/* open with file create */
#define	O_EXCL		_FEXCL		/* error on create if file exists */
#define O_NOCTTY	_FNOCTTY	/* POSIX don't give controlling tty */
#define	O_TRUNC		_FTRUNC		/* open with truncation */
#if	!defined(_POSIX_SOURCE)
#define O_BLKINUSE      _FBLKINUSE	/* block if "in use"	*0002*/
#define O_BLKANDSET     _FBLKANDSET	/* block, test and set "in use"	*/
#define O_FSYNC		_FSYNCRON	/* syncronous write *0004*/
#define O_TERMIO	_FTERMIO	/* termio style program */
#endif	/* NOT _POSIX_SOURCE */
/*
 * mask for use with file access modes
 */
#define	O_ACCMODE	(O_RDONLY|O_WRONLY|O_RDWR)
/*
 * Values for cmd used by fcntl()
 */
#define	F_DUPFD	0	/* Duplicate fildes */
#define	F_GETFD	1	/* Get fildes flags */
#define	F_SETFD	2	/* Set fildes flags */
#define	F_GETFL	3	/* Get file flags */
#define	F_SETFL	4	/* Set file flags */
#define	F_GETLK	7	/* Get file lock */
#define	F_SETLK	8	/* Set file lock */
#define	F_SETLKW 9	/* Set file lock and wait */
#if	!defined(_POSIX_SOURCE)
#define	F_GETOWN 5	/* Get owner */
#define F_SETOWN 6	/* Set owner */
#define F_SETSYN 10	/* Set syncronous write *0004*/
#define F_CLRSYN 11	/* Clear syncronous write *0004*/
#endif	/* NOT _POSIX_SOURCE */
/*
 * file descriptor flags used by fcntl(2)
 */
#define	FD_CLOEXEC 1	/* Close file descriptor on exec() */

/* file segment locking set data type - information passed to system by user */
#if !defined(F_RDLCK) || defined(POSIX)
struct flock {
	short	l_type;
	short	l_whence;
	off_t	l_start;
	off_t	l_len;		/* len = 0 means until end of file */
#ifndef	POSIX
	int	l_pid;
#else
	pid_t	l_pid;
#endif
};
#endif /* NOT F_RDLCK or POSIX */
/* 
 *	file segment locking types
 */
#define	F_RDLCK	01	/* Read lock */
#define	F_WRLCK	02	/* Write lock */
#define	F_UNLCK	03	/* Remove lock(s) */

#if	!defined(_POSIX_SOURCE)
/*
 * Flock call.
 */
#define	LOCK_SH		1	/* shared lock */
#define	LOCK_EX		2	/* exclusive lock */
#define	LOCK_NB		4	/* don't block when locking */
#define	LOCK_UN		8	/* unlock */
/*
 * Lseek call.
 */
#define	L_SET		0	/* absolute offset */
#define	L_INCR		1	/* relative to current offset */
#define	L_XTND		2	/* relative to end of file */

/*
 * Access call.
 * These are defined here for use by access() system call.  User level
 * programs should get these definitions from <unistd.h>. Included here
 * for historical reasons.
 */
#ifndef R_OK
#define	F_OK		0	/* does file exist */
#define	X_OK		1	/* is it executable by caller */
#define	W_OK		2	/* writable by caller */
#define	R_OK		4	/* readable by caller */
#endif /* R_OK */

#ifdef KERNEL
#define	GETF(fp, fd) { \
	if ((unsigned)(fd) >= NOFILE || ((fp) = u.u_ofile[fd]) == NULL) { \
		u.u_error = EBADF; \
		return; \
	} \
}
#define	DTYPE_INODE	1	/* file */
#define	DTYPE_SOCKET	2	/* communications endpoint */
#define	DTYPE_PORT	3	/* port (named pipe) 0003 */
#define DTYPE_PIPE	4	/* pipe */
#endif	/* KERNEL */

#endif	/* NOT _POSIX_SOURCE */

#if !defined(KERNEL) || defined(POSIX)
/*	Function declarations for X/Open compliance	*/
int	creat(), fcntl(), open();
#endif

#endif /* _FCNTL_H_ */
