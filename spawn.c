/*
 * usage: spawn tty prog [ args ... ]
 *     Start the specified process connected to the specified tty.
 *
 *     The main use for this is for emergency bootdisks where you do
 *   not want (or need) the overhead of getty/login.
 *     The idea is also to keep it as small as possible -- it will compile
 *   to 2 blocks as NMAGIC, or 4 blocks as a sparse QMAGIC file.
 *     This came originally from Jim Wiegand's `doshell', but since it
 *   is to be started from init, it doen't want to fork.  This has more
 *   error checking and reporting, and allows an arbitary number of
 *   arguments.  If you want it in the background, ask your shell nicely!
 *
 *		Nick Holloway, 15th May 1994.
 */
# include <fcntl.h>
# include <unistd.h>
# include <stdio.h>
# include <string.h>
# include <errno.h>
# include <sys/stat.h>

int main ( int argc, char *argv[] )
{
    int fd;
    char *file, argv0 [ 32 ];
    struct stat st;

    /* check we have enough arguments */
    if ( argc < 3 ) {
	fprintf ( stderr, "usage: %s tty prog [ args ... ]\n", argv[0] );
	exit ( 1 );
    }

    /* become our own process group, and lose any controlling tty */
    (void) setsid();

    /* check that we actually have a tty (well, character device is close) */
    if ( lstat ( argv[1], &st ) < 0 || ! S_ISCHR ( st.st_mode ) ) {
	fprintf ( stderr, "%s: %s: is not a tty\n", argv[0], argv[1] );
	exit ( 1 );
    }

    /* open the new tty, and connect it to std{in,out,err} */
    if ( ( fd = open ( argv[1], O_RDWR ) ) < 0 ) {
	fprintf ( stderr, "%s: %s: open: %s\n", argv[0], argv[1],
		strerror ( errno ) );
	exit ( 1 );
    }
    if ( ( fd != 0 && dup2 ( fd, 0 ) < 0 )
	    || ( fd != 1 && dup2 ( fd, 1 ) < 0 )
	    || ( fd != 2 && dup2 ( fd, 2 ) < 0 ) ) {
	fprintf ( stderr, "%s: can't redirect std{in,out,err}: %s\n", argv[0],
		argv[1], strerror ( errno ) );
	exit ( 1 );
    }
    if ( fd > 2 )
	close ( fd );

    /* shift arguments so that argv[0] refers to process to spawn */
    argv += 2;

    /* find basename... */
    if ( ( file = rindex ( argv[0], '/' ) ) != NULL )
	file++;
    else
	file = argv[0];

    /* ...and create an argv[0] so process will think it is a login shell */
    strncpy ( argv0 + 1, file, sizeof(argv0) - 2 );
    argv0 [ 0 ] = '-';
    argv0 [ sizeof(argv0) ] = '\0';

    /* perform switch around */
    file = argv [ 0 ];
    argv [ 0 ] = argv0;

    /* now try to run it... */
    execvp ( file, argv );

    /* ...or report error on failure (but it will be to the new tty)... */
    fprintf ( stderr, "%s: execv: %s: %s\n", argv[-2], file,
	    strerror ( errno ) );

    exit ( 1 );
}
