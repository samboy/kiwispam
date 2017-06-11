/* Public domain code by Sam Trenholme 2000, 2001 */
/* Routines that interface between the string library and the underlying
   OS */

/* Headers for the underlying OS calls */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/file.h>
#include <limits.h>
#include <unistd.h>
#ifdef THREADS
#include <pthread.h>
#endif /* THREADS */

/* Headers for the string routines */
#include "JsStr.h"

/* Structure that keeps track of allocated memory */
#ifdef DEBUG
leak_tracker *lt_head = NULL;
#endif /* DEBUG */
#ifdef THREADS
pthread_mutex_t alloc_lock = PTHREAD_MUTEX_INITIALIZER;
#endif /* THREADS */

/* js_alloc: Allocate memory from the underlying OS.
   input: The unit count and unit size of memory to allocate
   output: An anonymous pointer to the allocated memory */

void *js_alloc(int unit_count, int unit_size) {
    void *data;
    data = (void *)malloc(unit_count * unit_size);
    if(data == NULL) {
        /* Securty: In a situtation where we can not allocate memory,
	   the subsequent behavior of the program is undefined.  Hence, 
	   the best thing to do is exit then and there */
	printf("Aieeeeee, can not allocate memory!");
	exit(64);
        return (void *)0;
        }
    return data;
    }

#ifdef DEBUG
/* js_alloc_DEBUG: Allocate memory from the underlying OS, keeping track
                   of where the memory is being allocated from.
   input: The unit count and unit size of memory to allocate, a char string
          of why we are allocating the memory
   output: An anonymous pointer to the allocated memory
*/
void *js_alloc_DEBUG(int unit_count, int unit_size, char *whence) {
    void *data;
    leak_tracker *node, *point;
    char *why;
    data = (void *)malloc(unit_count * unit_size);
    if(data == NULL) {
        printf("Can not allocate memory");
	exit(64);
	return (void *)0;
	}
    /* Now, add an element to the leak tracker linked list */
    node = (leak_tracker *)malloc(sizeof(leak_tracker));
    if(node == NULL) {
        printf("Can not allocate lt memory");
	exit(64);
	free(data);
	return (void *)0;
	}
    why = (char *)malloc(strlen(whence) + 3);
    if(why == NULL) {
        printf("Can not allocate ltw memory");
	exit(64);
	free(data);
	free(node);
	return (void *)0;
	}
    strncpy(why,whence,strlen(whence) + 1);
    node->where = data;
    node->why = why;
    node->next = NULL;
#ifdef THREADS
    pthread_mutex_lock(&alloc_lock);
#endif /* THREADS */
    /* Put the new element at the end of the linked list */
    if(lt_head == NULL) 
        lt_head = node;
    else {
        point = lt_head;
	while(point->next != NULL)
	    point = point->next;
        point->next = node;
	}
#ifdef THREADS
    pthread_mutex_unlock(&alloc_lock);
#endif /* THREADS */
    /* Finally, return the pointer to the memory the calling function
       wanted */
    return data;
    }
#else  /* DEBUG */
#define js_alloc_DEBUG(x,y,z) js_alloc(x,y)
#endif /* DEBUG */

/* js_dealloc: Deallocate memory from the underlying OS
   input: A pointer to the memory we wish to free
   output: -1 on failure, 1 on success */

int js_dealloc(void *pointer) {
#ifdef DEBUG
    leak_tracker *npoint, *point, *last;
    /* If we are keeping track of allocated memory, remove the element from
       the linked list indicating that we have allocated memory */
#ifdef THREADS
    pthread_mutex_lock(&alloc_lock);
#endif /* THREADS */
    point = lt_head;
    last = NULL;
    while(point != NULL) {
	npoint = point->next;
        /* If this is the node for the allocated memory in question, remove
	   the node */
        if(point->where == pointer) {
	    free(point->why);
	    /* We have to handle the special case of the node being
	       the head of the linked list */
	    if(last == NULL) 
	        lt_head = point->next;
            /* This is the normal case */
            else 
	        last->next = point->next;
            free(point);
	    }
        else
            last = point; 
        point = npoint;
        }
#ifdef THREADS
    pthread_mutex_unlock(&alloc_lock);
#endif /* THREADS */
#endif /* DEBUG */
    free(pointer);
    return 1;
    }

#ifdef DEBUG
/* Function which prints to the standard output all unallocated strings which
   we have a record of */
int js_show_leaks() {
    leak_tracker *point;
    point = lt_head;
    while(point != NULL) {
        printf("%s\n",point->why);
	point = point->next;
	}
    }
#else  /* DEBUG */
#define js_show_leaks()
#endif /* DEBUG */

/* js_show_stdout: Display the contents of a given js_string
                          object on standard output
   input: Pointer to js_string object
   output: -1 on failure, 1 on success */
int js_show_stdout(js_string *js) {
    int counter = 0;

    if(js_has_sanity(js) < 0) 
        return -1;

    while(counter < js->unit_size * js->unit_count) {
        putc(*(js->string + counter),stdout);
        counter++;
        }
    
    return 1;
    }

/* js_getline_stdin: Get a line from the standard input
   input: To to js_string object to put stdin contents in
   output: JS_ERROR on error, JS_SUCCESS on success.
   note: js->encoding needs to be set to something besides JS_BINARY */
int js_getline_stdin(js_string *js) {
    char *temp; /* Temporary place to put chars until they become
                   a part of js */
    js_string *newlines, *append;
    int counter = 0;
    temp = js_alloc(js->unit_size,1);
    newlines = js_create(256,js->unit_size);
    if(newlines == 0)
        return JS_ERROR;
    newlines->encoding = js->encoding;
    append = js_create(256,js->unit_size);
    if(append == 0) {
        js_destroy(newlines);
        js_dealloc(temp);
        return JS_ERROR;
        }
    js_newline_chars(newlines);
    if(js_has_sanity(js) == JS_ERROR)
        goto error;
    if(temp == 0) {
        js_destroy(newlines);
	js_destroy(append);
        js_dealloc(temp);
        return JS_ERROR;
        }
    /* Blank out the js string */
    js->unit_count = 0; 
    while(!feof(stdin) && js_match(newlines,js) == -2) {
        temp[counter] = getc(stdin);
        counter++;
	if(counter >= js->unit_size) {
	    counter = 0;
	    js_str2js(append,temp,1,js->unit_size);
	    if(js_append(append,js) == JS_ERROR)
                goto error;
	    }
        }

    /* Success! */
    js_destroy(append);
    js_destroy(newlines);
    js_dealloc(temp);
    return JS_SUCCESS;

    error:
        js_destroy(append);
        js_destroy(newlines);
        js_dealloc(temp);
        return JS_ERROR;
    }

/* js_open: Open a file 
   input:  js_string pointing to file we wish to open, flags
   output: void */
void js_open(js_string *filename, js_file *desc, int flags) {
    char temp[256];

    /* Return if the length of the string is greater than
       the space we allocated for the filename */
    if(filename->unit_count * filename->unit_size > 255) {
        desc->filetype = -1;
	return;
        } 

    /* Copy over the filename to the temp string to make it a
       null terminated string */

    js_js2str(filename,temp,255);

    desc->filetype = JS_OPEN2;
    desc->file_desc = open(temp,flags,00600);
    desc->number = -1; /* You can set up buffering later if you want */
    desc->eof = 0; /* Are we at the end of file? */
    desc->buffer = 0;

    if(desc->file_desc == -1) {
        desc->filetype = -1;
	return;
	}

    return;

    } 

/* js_open_append: Open a file for appending
   input:  js_string pointing to file we wish to open
   output: JS_SUCCESS on success, JS_ERROR on error */
int js_open_append(js_string *filename, js_file *desc) {
    js_open(filename,desc,O_WRONLY | O_APPEND | O_CREAT);
    if(desc->filetype == -1)
        return JS_ERROR;
    else
        return JS_SUCCESS;
    } 

/* js_open_write: Open a file for writing
   input:  js_string pointing to file we wish to open
   output: JS_SUCCESS on success, JS_ERROR on error */
int js_open_write(js_string *filename, js_file *desc) {
    js_open(filename,desc,O_WRONLY | O_CREAT);
    if(desc->filetype == -1)
        return JS_ERROR;
    else
        return JS_SUCCESS;
    } 

/* js_open_read: Open a file for reading
   input:  js_string pointing to file we wish to open
   output: JS_SUCCESS on success, JS_ERROR on error */
int js_open_read(js_string *filename,js_file *desc) {
    js_open(filename,desc,O_RDONLY);
    if(desc->filetype == -1)
        return JS_ERROR;
    else
        return JS_SUCCESS;
    } 

/* js_rewind: Rewind to the beginning of a given file
   input: File descriptor
   output: JS_ERROR (-1) on error, JS_SUCCESS on success */ 
int js_rewind(js_file *desc) {
 
    if(desc == 0)
        return JS_ERROR;
    if(desc->filetype != JS_OPEN2) 
        return JS_ERROR;

    /* Get rid of the buffer */
    if(desc->buffer != 0)
        js_destroy(desc->buffer);
    desc->number = -1;

    if(lseek(desc->file_desc,0,SEEK_SET) == -1)
        return JS_ERROR;

    return JS_SUCCESS;

    }

/* js_read: Read n bytes from a file, and place those bytes in a js_string
            object.
   input: File descriptor, js_string object, bytes to read
   output: JS_ERROR (-1) on error, bytes read on success */ 
int js_read(js_file *desc, js_string *js, int count) {
  
    ssize_t value;
    int ret;

    /* Sanity checks */
    if(js_has_sanity(js) == JS_ERROR)
        return JS_ERROR;
    if(desc->filetype != JS_OPEN2) 
        return JS_ERROR;
    if(count % js->unit_size != 0)
        return JS_ERROR;
    if(count < 0 || count > SSIZE_MAX)
        return JS_ERROR;
    
    if(count > js->unit_size * js->max_count)
        return JS_ERROR;

    value = read(desc->file_desc,js->string,count);

    if(value == -1)
        return JS_ERROR;

    ret = value;

    if(ret % js->unit_size != 0) {
         js->unit_count = 0;
         return JS_ERROR;
         }

    js->unit_count = ret / js->unit_size;

    return ret;

    }

/* js_write: Write from a js_string object in to a file 
   input: File descriptor, js_string object
   output: JS_ERROR (-1) on error, JS_SUCCESS on success */ 
int js_write(js_file *desc, js_string *js) {
  
    ssize_t value;
    int to_write,written;

    /* Sanity checks */
    if(js_has_sanity(js) == JS_ERROR)
        return JS_ERROR;
    if(desc->filetype != JS_OPEN2) 
        return JS_ERROR;
    
    to_write = js->unit_size * js->unit_count;
    if(to_write < 0 || to_write > SSIZE_MAX)
        return JS_ERROR;

    value = write(desc->file_desc,js->string,to_write);

    written = value;

    if(written != to_write)
        return JS_ERROR;

    return JS_SUCCESS;

    }

/* js_close: Close an opened file
   input: js file descriptor
   output: JS_ERROR (-1) on error, JS_SUCCESS (1) on success */
int js_close(js_file *desc) {
    int ret;

    /* Close the file, making sure we closed it */
    ret = close(desc->file_desc);

    /* Get rid of the buffer */
    if(desc->buffer != 0)
        js_destroy(desc->buffer);
    desc->number = -1;

    if(ret == -1)
        return JS_ERROR;

    /* Change the filetype of the closed file to an invalid filetype */
    desc->filetype = JS_ERROR;

    return JS_SUCCESS;

    }

/* Since Solaris has problems with BSD libraries (ugh), and since MaraDNS
   does not currently use file locking, I am, against my better judgment, 
   disabling this if SOLARIS is defined, to make compiling easier */

#ifndef SOLARIS

/* js_lock: Lock a file using flock()
   input: Pointer to file desc
   output: JS_ERROR on error, JS_SUCCESS on success */
int js_lock(js_file *desc) {

    /* Lock the file: This may freeze here waiting for lock */
    flock(desc->file_desc,LOCK_EX);

    return JS_SUCCESS;

    }

/* js_unlock: Unlock a file using flock()
   input: Pointer to file desc
   output: JS_ERROR on error, JS_SUCCESS on success */
int js_unlock(js_file *desc) {

    /* Lock the file: This may freeze here waiting for lock */
    flock(desc->file_desc,LOCK_UN);

    return JS_SUCCESS;

    }

#endif /* SOLARIS, see commentary above */

/* js_buf_eof: Tell us if we hit the end of a given file
   input: Pointer to file descriptor
   output: 0 if not end of file, otherwise 1 */
int js_buf_eof(js_file *desc) {
    if(desc->eof && desc->number >= desc->buffer->unit_count)
        return 1;
    return 0;
    }

/* js_buf_read: Read a new chunk of the file and put it in the buffer 
   input: Pointer to a file descriptor
   output: JS_ERROR (-1) on error, JS_SUCCESS(1) on success */
int js_buf_read(js_file *desc) {
    int bytes_read;
    
    /* Sanity check */
    if(js_has_sanity(desc->buffer) == -1)
        return -1;
    bytes_read = js_read(desc,desc->buffer,
                         JS_BUFSIZE * desc->buffer->unit_size);
    if(bytes_read != JS_BUFSIZE * desc->buffer->unit_size)
        desc->eof = 1;
    desc->number = 0;
    
    return JS_SUCCESS;
    }
 
/* js_buf_getline: Grab a line from an opened file (uses buffering)
   input: Pointer to file descriptor of open file, pointer to js_string to
          put line in
   output: JS_ERROR (-1) on error, JS_SUCCESS(1) on success */
int js_buf_getline(js_file *desc, js_string *js) {
    js_string *newlines, *temp;
    int next_newln;
    int overflowed = 0;

    /* Sanity check */
    if(js_has_sanity(js) == -1)
        return -1;

    /* This is tricky to do because we have to do our own buffering */
    /* If the buffer is not there yet, create one */
    if(desc->number == -1) {
        if(desc->buffer == 0)
            desc->buffer = js_create(JS_BUFSIZE + 10,js->unit_size);
        js_buf_read(desc);
        }

    /* Sanity check */
    if(js->unit_size != desc->buffer->unit_size) 
        return -1;

    /* Make an js_string object that is all of the allowed newline 
       characters */
    newlines = js_create(js->max_count,js->unit_size);
    js_copy(js,newlines);
    js_newline_chars(newlines);

    /* Find the next newline character in the string */
    next_newln = js_match_offset(newlines,desc->buffer,desc->number);
    /* If we are at the end of the buffer in question w/o finding a 
       newlines...*/
    if(next_newln == -2 && desc->eof == 0) {
        /* ...then we put the rest of the buffer in js, but only if
           it will fit in js.  Otherwise, we blank out js and return
           success. */ 
        if(JS_BUFSIZE + 1 - desc->number < js->max_count && overflowed == 0)
            js_substr(desc->buffer,js,desc->number,
                      JS_BUFSIZE - desc->number);
        else {
            js_str2js(js,"",0,js->unit_size); /* blank line if overflow */
            overflowed = 1;
            }
        /* And load up a new buffer */
        js_buf_read(desc);
        /* If the new buffer does not have a newline in it, and is a full-sized
           buffer, then we handle this special case */
        while(js_match(newlines,desc->buffer) == -2 && desc->eof == 0) {
            if(js->unit_count + JS_BUFSIZE < js->max_count && overflowed == 0)
                js_append(desc->buffer,js);
            else {
                js_str2js(js,"",0,js->unit_size);
                overflowed = 1;
                }
            js_buf_read(desc);
            }
        next_newln = js_match(newlines,desc->buffer);
        temp = js_create(js->max_count,js->unit_size);
        js_substr(desc->buffer,temp,0,next_newln + 1);
        js_append(temp,js);
        js_destroy(temp);
        if(next_newln != -2)
            desc->number = next_newln + 1;
        if(desc->number >= JS_BUFSIZE)
            js_buf_read(desc);
        js_destroy(newlines);
        return JS_SUCCESS;
        }
    else if(next_newln == -2) {
        /* We have EOF.  Read the string and return it */
	/* With overflow checking, of course */
        if(desc->buffer->unit_count - desc->number < js->max_count 
           && overflowed == 0)
	    js_substr(desc->buffer,js,desc->number,
	              desc->buffer->unit_count - desc->number);
        else {
            js_str2js(js,"",0,js->unit_size);
            overflowed = 1;
            }

        /* Make sure js_buf_eof sees it as an EOF */
        desc->number = desc->buffer->unit_count + 1;

        js_destroy(newlines);
	return JS_SUCCESS;
        }

    /* At this point, we can assume that we found a newline in the buffer */
    if(next_newln + 1 - desc->number < js->max_count) {
        js_substr(desc->buffer,js,desc->number,next_newln + 1 - desc->number);
        desc->number = next_newln + 1;
        if(desc->number >= JS_BUFSIZE)
            js_buf_read(desc);
        }
    else
        js_str2js(js,"",0,js->unit_size);

    js_destroy(newlines);
    return JS_SUCCESS;

    }

/* js_qstr2js: "Quick" version of js_str2js routine.
   input: pointer to js object, NULL-terminated string
   output: JS_ERROR on failure, JS_SUCCESS on success 
   note: it's here because of strlen call */
int js_qstr2js(js_string *js, char *string) {
    return js_str2js(js,string,strlen(string),js->unit_size);
    }

/* js_adduint32: Add a 32-bit number to the end of a js_string obejct 
                 in big-endian format, where the js->unit_size is one.
   input: pointer to js_string object, number to add
   output: JS_ERROR on error, JS_SUCCESS on success
   (This is in OS because of the dependency on u_int32_t
*/

int js_adduint32(js_string *js, u_int32_t number) {

    /* sanity checks */
    if(js_has_sanity(js) == JS_ERROR)
        return JS_ERROR;
    if(js->unit_size != 1)
        return JS_ERROR;
    /* No buffer overflows */
    if(js->unit_count + 4 >= js->max_count)
        return JS_ERROR;

    /* Add the uint16 to the end of the string */
    *(js->string + js->unit_count) = (number >> 24) & 0xff;
    *(js->string + js->unit_count + 1) = (number >> 16) & 0xff;
    *(js->string + js->unit_count + 2) = (number >> 8) & 0xff;
    *(js->string + js->unit_count + 3) = number & 0xff;
    js->unit_count += 4;

    return JS_SUCCESS;
    }

/* js_readuint32: Read a single uint32 (in big-endian format) 
                  from a js_string object
   input: pointer to js_string object, offset from beginning
          of string (0 is beginning of string, 1 second byte, etc.)
   output: JS_ERROR on error, value of uint32 on success
           (Hack: 0xffffffff is the same as -1 in comparisons)
   (This is in OS because of the dependency on u_int32_t)
*/

u_int32_t js_readuint32(js_string *js, unsigned int offset) {

    u_int32_t ret;
    /* sanity checks */
    if(js_has_sanity(js) == JS_ERROR)
        return 0xffffffff;
    if(js->unit_size != 1)
        return 0xffffffff;
    if(offset > (js->unit_count - 4) || offset < 0)
        return 0xffffffff;

    ret = ((*(js->string + offset) << 24) & 0xff000000) |
          ((*(js->string + offset + 1) << 16) & 0xff0000) |
          ((*(js->string + offset + 2) << 8) & 0xff00) |      
           (*(js->string + offset + 3) & 0xff);

    /* Make sure we do not inadvertently return JS_ERROR */
    if(ret == 0xffffffff)
        ret = 0xfffffffe;

    return ret;

    }


