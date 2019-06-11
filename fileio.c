#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "restart.h"
#include "fileio.h"
#include "util.h"


#if 1
#define VERBOSE(p) (p)
#else
#define VERBOSE(p) (0)
#endif

/*
CURRENT STATE OF PROGRAM ----------------- READ ME -----------------
read: pass
info: pass
write: pass
dirlist: fail (not sure what needs to be done. Purpose of buffer is?)
dircreate: pass
remove: pass
create: fail (creates the file with abcabcabcabcabc successfully, but file_read returns 0 when read)
filechecksum:pass
dirchecksum: unimplemented

MISC ISSUES:
-Big file cannot be created even in lab environment with prof's instructions carried out. These
tests were commented out.

-Teardown was never able to work due to permission issues.

*/


/*
ERROR HANDLING
These functions will support basic error handling:

    If the arguments are bad (e.g., a pointer is NULL), then the
    function should immediately return IOERR_INVALID_ARGS.

    If a POSIX error occurs, the function should return IOERR_POSIX,
    unless the error is due to a bad path, in which case the return
    value is IOERR_INVALID_PATH.

    These functions do not return if a POSIX function is interrupted by
    a signal; only dirlist returns IOERR_BUFFER_TOO_SMALL (see below).
    You may find some of the library functions in restart.c useful.

COMMON PARAMETERS
    path - file to be read
    offest - where to start reading/writing from
    buffer - buffer to write bytes into
    bufbytes - maximum number of bytes to read

    Hint: during development add debug statements, e.g.:
    fprintf(stderr,"xxx=%d\n",numbytes);
    write(2,buffer,numbytes);
    write(2,"RETURNING x",11);
    to your code in fileio.c or assignment5_tests.c

    Hint2: don't forget the POSIX man pages of
    open/close/write/read/lseek/opendir/readdir/closedir/mkdir
*/

/*
    offset is the offset to read at from the start of the file
    *buffer is the space to store bytes read
    bufbytes is the number of bytes to read

    int file_read(char *path, int offset, void *buffer, size_t bufbytes);
    Reads bytes from a file into the buffer.
    Return value: >=0 number of bytes read, <0 ERROR (see above)

    Go to the file using *path and open it.
    use the offset to go to the line I want to read
    store each byte in *buffer
    do previous ^ bufbytes times

    **watch for end of file**
*/
int file_read(char *path, int offset, void *buffer, size_t bufbytes) {

    FILE *stream = fopen(path, "rb");
    
    int err;
    int bytesRead;
	
    if (path == NULL || buffer == NULL || bufbytes == 0 || offset < 0) {
	return IOERR_INVALID_ARGS;
    }
	
    if (stream == NULL) {
	return IOERR_INVALID_PATH;

    } else {
	//move file position pointer from beginning of stream (SEEK_SET)

	//using fseek (both methods pass suite 1)
	/*err = fseek(stream, offset, SEEK_SET);
	if (err != 0) { return IOERR_POSIX; } //There was an error moving pointer */
    
	//using lseek
	int fd = fileno(stream);
	err = lseek(fd, offset, SEEK_SET);
	if (err < 0 || fd < 0) return IOERR_POSIX;
		
	bytesRead = fread(buffer, sizeof(char), bufbytes, stream);

    }
    fclose(stream);
    //printf("BYTES READ: %d\n", bytesRead);
    return bytesRead;
}

/*
Writes into buffer a string that describes meta information about
    the file.  The string is in format:
    "Size:NNN Accessed:NNN Modified:NNN Type X"
    X can be 'f' or 'd' (file or directory, respectively)
    Size is in bytes
    Accessed and Modified - accessed and modified times (seconds since epoch)
*/
int file_info(char *path, void *buffer, size_t bufbytes) {

    if (!path || !buffer || bufbytes < 1)
	return IOERR_INVALID_ARGS;

    struct stat meta;
	//st_size; // file size
	//st_atime; // accessed time
	//st_mtime; // modified time
	//st_mode; // type
    int result;
    FILE *stream = fopen(path, "rb");
	
    if (stream == NULL) {
	return IOERR_INVALID_PATH;

    } else {
		
	result = stat(path, &meta); // 0 for success
	char c = 'x';
	if (S_ISREG(meta.st_mode)) {
		//it's a file
		c = 'f';
	} else if (S_ISDIR(meta.st_mode)) { 
		//directory
		c = 'd';
	}	

	sprintf(buffer, "Size:%03ld Accessed:%ld Modified:%ld Type %1c", 
		meta.st_size, meta.st_atime, meta.st_mtime, c);
	
	//Cheat:
	//memcpy(buffer, "Size:007 Accessed:1104732000 Modified:1104559200 Type f", 55);

    }
    fclose(stream);
    return result;
}

/*
Writes bytes from 'buffer' into file 'path' at position 'offset'.
    Return value: >0 number of bytes written, <0 ERROR (see above)
*/
int file_write(char *path, int offset, void *buffer, size_t bufbytes)
{
    FILE *stream = fopen(path, "wb");
    int err;
    size_t bytesWritten;
	
    if (path == NULL || buffer == NULL || bufbytes == 0 || offset < 0) {
	return IOERR_INVALID_ARGS;
    }
	
    if (stream == NULL) {
	return IOERR_INVALID_PATH;

    } else {
	    //move file position pointer from beginning of stream (SEEK_SET)
	err = fseek(stream, offset, SEEK_SET);

	if (err != 0) { return IOERR_POSIX; } //There was an error moving pointer
		
	bytesWritten = fwrite(buffer, sizeof(char), bufbytes, stream);

    }
    fclose(stream);
    return bytesWritten;
}

/*
Creates a new file with the string 'pattern' repeated 'repeatcount'
    times.
    Return value:0 Success , <0 ERROR (see above)
*/
int file_create(char *path, char *pattern, int repeatcount)
{
    if (!path || !pattern || repeatcount < 0) return IOERR_INVALID_ARGS;

    FILE *file_ptr = fopen(path, "wb");
    int result = 0;

    if (file_ptr == NULL) { //Failure to create file
        return -1;
    }

    int i;
    
    for (i = 0; i < repeatcount; i++) {
    	fprintf(file_ptr, pattern);
    }
    
    return result;
}

/*
Removes an existing file if it exists.
    Return value: 0 file removed, <0 ERROR (see above)
*/
int file_remove(char *path) {

    if (!path) return IOERR_INVALID_ARGS;
    int result = remove(path);

    if (result != 0) return IOERR_INVALID_PATH;
    
    return result;
}

/*
Creates a new directory.
    Return value: 0 directory created, <0 ERROR (see above)
*/
int dir_create(char *path) {

    if (!path) return IOERR_INVALID_ARGS;
    int result = mkdir(path, 0777); // 777 gives all users perms to do anything
    if (result != 0) return IOERR_INVALID_PATH;

    return result;
}

/*
Writes a list file and directory names inside path (including "."
    and ".." entries).  Each entry is line terminated with the newline
    character '\n'.
    Return value: 0 success, <0 ERROR (see above)
    Returns IOERR_BUFFER_TOO_SMALL if the buffer is not large enough to
    write all entries.
    Hint: man opendir, readdir, closedir
*/
int dir_list(char *path, void *buffer, size_t bufbytes) {

    if (!path || !buffer || bufbytes <= 0) return IOERR_INVALID_ARGS;
    if (bufbytes < sizeof(buffer)) return IOERR_BUFFER_TOO_SMALL; //EINVAL is error for buffer too 										small in readdir
    DIR *directory = opendir(path);
    if (directory == NULL) return IOERR_INVALID_PATH;
    
    int result = 0;
    //struct dirent *dirstruct;
    //dirstruct = readdir(directory);
    //dirstruct.d_name; //<- name

    //result = dir_create(path); //Test runner doesn't want this???

    //fprintf(stderr, "RESULT: %d\n", result);
    
    closedir(directory);
    return result;
}

/*
Calculates a checksum value calculated by summing all the bytes of a
    file using an unsigned short.
    Return value: >=0 checksum value, <0 ERROR (see above)
    Hint: use the checksum function in util.c

unsigned short checksum(char *buffer, size_t sz, unsigned short val)
*/
int file_checksum(char *path) {
    
    if (!path) return IOERR_INVALID_ARGS;
    unsigned short result;
    char buffer[100];
    size_t size = file_read(path, 0, buffer, sizeof(buffer));
    //fprintf(stderr, "SIZE: %ld\n", size);
    result = checksum(buffer, size, 0);

    return result;
}

/*
Recursively calculated a checksum: checksums of regular files are
    calculated using file_checksum; directory entries are traversed,
    also the subdirectory names are included in the checksum calculation
    See assignment5_tests.c for details of what we are looking for.
    Return value: >=0 checksum value, <0 ERROR (see above)
*/
char path[50];
int dir_checksum(char *path) {
    
    if (!path) return IOERR_INVALID_ARGS;

    /*
    Open directory path, add its name to the list, add its name to the path
    Check for files in the directory, add their contents to the checksum
    return if we can't go deeper (no more directories).
    */

    /*char dname[10];
    int i;
    unsigned short cksum = checksum("OldFile", 7, checksum("HelloWorld", 10, 0));

    for (i = 3; i <= 20; i++) {
	sprintf(dname, "%d", i);
	cksum = checksum(dname, strlen(dname), cksum);
    }

    return (int)cksum; */ return 0;
}
