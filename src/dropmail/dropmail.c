/* Put in the public domain 2000 by Sam Trenholme */
/* Kiwi Parse: A series of programs that parse the .kiwirc file and set the
   relevent global variables */

#include "../JsStr/JsStr.h"
#include "dropmail.h"
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <sys/types.h>

/* find_kiwirc: Find the kiwirc file we are supposed to read
           Input: js_string to place kiwirc file in
           Output: JS_ERROR on error, JS_SUCCESS on success
*/
int main() {
  
    struct passwd *info;
    js_string *username, *mailplace, *buffer;
    js_file *desc;
    char byte[4150];
    int found = 0, counter;
    uid_t uid;

    if((username = js_create(256,1)) == 0) {
        fprintf(stderr,"%s","Unable to create username string\n");
        exit(2);
        }
    if((mailplace = js_create(256,1)) == 0) {
        fprintf(stderr,"%s","Unable to create mailplace string\n");
        exit(2);
        }
    if((buffer = js_create(4100,1)) == 0) {
        fprintf(stderr,"%s","Unable to create buffer string\n");
        exit(2);
        }
    desc = js_alloc(1,sizeof(js_file));
    if(desc == 0) {
        fprintf(stderr,"Unable to allocate memory for file in main\n");
        exit(2);
        }

    /* Get the username in /etc/passwd */
    info = getpwent();
    uid = getuid();
    while(info != NULL) {
        if(info->pw_uid == uid) { /* Bingo */
            if(js_qstr2js(username,info->pw_name) == JS_ERROR) {
                fprintf(stderr,"%s","Unable to convert username\n");
	        exit(3);
                }
            found = 1;
            break;
            }
        info = getpwent();
        }

    /* Get the directory the mail is in */
    if(js_qstr2js(mailplace,MAILDROP) == JS_ERROR) {
        fprintf(stderr,"%s","Unable to get maildrop string\n");
        exit(4);
        }
  
    /* Add the username to the end of the mail directory name */ 
    if(js_append(username,mailplace) == JS_ERROR) {
        fprintf(stderr,"%s","Unable to append usename to mailplace\n");
        exit(5);
        } 

    /* Open up the file in question for writing */ 
    if(js_open_append(mailplace,desc) == JS_ERROR) {
        fprintf(stderr,"%s","Unable to open mailbox for appending\n");
	exit(6);
	}

    /* Lock the file, to minimize the chance of corruption */
    js_lock(desc);
    
    /* Append the standard output to the file (mailbox) in question */
    while(!feof(stdin)) {
        counter = 0;
        while(counter < 4096 && !feof(stdin)) {
            byte[counter] = getc(stdin);
	    counter++;
	    }
        if(feof(stdin))
            counter--;
        if(counter > 0) {
            js_str2js(buffer,byte,counter,1);
            if(js_write(desc,buffer) == JS_ERROR)
                exit(10); /* Problem writing to buffer */
            }
        }

    /* Unlock the mailbox */
    js_unlock(desc);

    /* And clean things up */
    js_close(desc);

    exit(0); /* Sucessful mail appending */
    }

