/* Put in the public domain 2000 by Sam Trenholme */
/* Kiwi Main:  The part of the Kiwi rouines with main() in it and the
   relevent command line parsers */

#include "JsStr/JsStr.h"
#include "JsStr/Kiwi.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

/* Function prototypes for void functions */
void gencookie();
void decode();
void wrapper();
void infilter();
void bounce();
void deliver();
void forward();
void piper();
void append();

/* Alphanumberic digits */
#define AN "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-_"

/* Size of cookie in octets */
#define COOKIE_SIZE 7

/* Softerror: Exit with an error code allowing the MTA to try again */
int softerror(char *errormsg) {
    fprintf(stderr,"Soft error: %s\n",errormsg);
    exit(111); /* Exit code 111: Try again later */
    }

/* Harderror: Exit with an error code NOT allowing the MTA to try again */
/* As a general rule, use softerror when manipulations on non-user-controlled
   data go haywire, and use harderror when manipulations of user-controlled
   data go haywire */
int harderror(char *errormsg) {
    fprintf(stderr,"Fatal error: %s\n",errormsg);
    exit(100); /* Exit code 100: Return to sender */
    }

int main(int argc, char **argv) {
    js_string *arg, *tstr, *tstr2, *kiwirc;
    char badstr[256];
    char *envget; 
    int shift = 0;
    int tnum = 0;
    int offset = 0;

    /* Initialize js_string objects */
    if((arg = js_create(256,1)) == 0) 
        softerror("Unable to allocate arg.  Try later.");
    js_set_encode(arg,JS_US_ASCII);
    if((tstr = js_create(256,1)) == 0) 
        softerror("Unable to allocate tstr.  Try later.");
    js_set_encode(tstr,JS_US_ASCII);
    if((tstr2 = js_create(256,1)) == 0) 
        softerror("Unable to allocate tstr2.  Try later.");
    js_set_encode(tstr2,JS_US_ASCII);
    if((kiwirc = js_create(256,1)) == 0) 
        softerror("Unable to allocate kiwirc.  Try later.");
    js_set_encode(kiwirc,JS_US_ASCII);
    
    /* Regardless of anything else, we do need to read the user's .kiwirc
       file */
    if(find_kiwirc(kiwirc) == JS_ERROR)
        softerror("Error trying to locate .kiwirc file.");
    /* To do: Make sure the .kiwirc file has sane perms (600 or 700) */
    if(read_kiwirc(kiwirc,tstr,&tnum) == JS_ERROR)
        softerror("Error in reading .kiwirc file");
    if(tnum != 0) { /* If there was a syntax error in the .kiwirc file */
        if(tnum < -9000 || tnum > 9000)
	    softerror("Error number is out of bounds reading .kiwirc");
        snprintf(badstr,100,"%d",tnum); /* I really need a js_itoa function */
        js_qstr2js(tstr2,"Syntax error in .kiwirc file: ");
        js_append(tstr,tstr2); /* Add the error string */
        js_qstr2js(tstr,", line ");
        js_append(tstr,tstr2);
        js_qstr2js(tstr,badstr); /* Add the line number the error was on */
        js_append(tstr,tstr2);
        js_js2str(tstr2,badstr,100); /* Make it a NULL-terminated string */
        softerror(badstr);
        } 

    /* If argc is less than one (no arguments except argv[0]) we're haywire */
    if(argc < 1) 
        harderror("Argc is less than one!");
       
    /* Look at argv[0] and act accordingly */
    if(argv[0] != 0) {
        if(js_qstr2js(arg,argv[0]) == JS_ERROR) 
            harderror("Can not parse argv[0].");
        }
    else 
        harderror("argv[0] is a NULL pointer!");

    /* Now then, let's see how we should run Kiwi */

    /* Ignore anything leading up to a / */
    offset = 0;
    do {
        if(js_qstr2js(tstr,"/") == JS_ERROR)
            softerror("Can not give tstr a value, strange.");

        tnum = js_match_offset(tstr,arg,offset);
        if(tnum == JS_ERROR)
            harderror("Unable to run match on argv[0].");
    
        if(tnum != -2)
            offset = tnum + 1;

        } while(tnum != -2);

    /* See if the first letter of the command is an "i" (infilter) */

    if(js_qstr2js(tstr,"i") == JS_ERROR)
        softerror("Cannot give tstr a value!");

    tnum = js_match_offset(tstr,arg,offset);
    if(tnum == JS_ERROR)
        harderror("Unable to run match on argv[0]");
    if(tnum == offset) /* Mail filter for incoming mail */ {
	infilter(argc,argv);
        exit(0);
	}

    /* See if the first letter of the command is a "c" */

    if(js_qstr2js(tstr,"c") == JS_ERROR) 
        softerror("Can not give tstr a value, strange.");

    tnum = js_match_offset(tstr,arg,offset);

    if(tnum == JS_ERROR)
        harderror("Unable to run match on argv[0].");
    if(tnum == offset) /* Command-line cookie generation */ {
        envget = (char *)getenv("REMOTE_ADDR");
        if(argc == 1) /* No arguments (execpt argv[0]) */
            gencookie(envget); /* This will be NULL if REMOTE_ADDR not set */	
        else if(argc == 2) { /* One argument */
            gencookie(argv[1]);
	    printf("\n");
	    }
	exit(0);
        } 

    /* See if the first letter of the command is a "d" */

    if(js_qstr2js(tstr,"d") == JS_ERROR)
        softerror("Can not give tstr a value");

    tnum = js_match_offset(tstr,arg,offset);
    if(tnum == JS_ERROR)
        harderror("Unable to run match on argv[0]");
    if(tnum == offset) /* Decode cookie */ {
        if(argc == 1)
	    decode(0);
        else if(argc == 2) 
	    decode(argv[1]);
        exit(0);
	}

    /* See if the first letter of the command is a "w" (wrapper) */

    if(js_qstr2js(tstr,"w") == JS_ERROR)
        softerror("Can not give tstr a value!");

    tnum = js_match_offset(tstr,arg,offset);
    if(tnum == JS_ERROR)
        harderror("Unable to run match on argv[0]");
    if(tnum == offset) /* Sendmail wrapper for outgoing mail */ {
	wrapper(argc,argv);
        exit(0);
	}

    harderror("Invalid argv[0] value.");

    }

/* data_to_ip: Convert 32-bit unsigned int to null-terminated dotted
   notation IP.
   input: 32-bit IP to convert, preallocatedstring to write in
   output: converted IP
*/

char *data_to_ip(Kiwi_cdata ip, char *string) {
    int tnum;
    /* Cut and paste from Kiwi version 1 */
    u_int8_t ipo[4];
    for(tnum=3;tnum>=0;tnum--) {
        ipo[tnum] = ip & 0xff;
        ip >>= 8;
        }
    snprintf(string,100,"%d.%d.%d.%d",ipo[0],ipo[1],ipo[2],ipo[3]);
    return string;
    }

/* data_to_five: Convert 32-bit unsigned int containing a kiwi-encoded
   five letter message to the message in question
   input: message to convert, preallocatedstring to write in
   output: converted message
*/
char *data_to_five(Kiwi_cdata data, char *mesg) {
    int tnum;
    for(tnum=4;tnum>=0;tnum--) {
        *(mesg + tnum)=five_letter_map(data & 31);
        data >>= 5;
        }
    *(mesg + 5)=0;
    return mesg;
    }

void decode(char *crypto) {
    js_string *cookie, *key, *format, *tstr;
    Kiwi_cdata data;
    char string[256];
    int counter, tnum;

    if((format = js_create(256,1)) == 0)
        softerror("Can not allocate temp string in gencookie");
    js_set_encode(format,JS_US_ASCII);
    if((tstr = js_create(256,1)) == 0)
        softerror("Can not allocate temp string in gencookie");
    js_set_encode(tstr,JS_US_ASCII);
    if((cookie = js_create(256,1)) == 0)
        softerror("Can not allocate temp string in gencookie");
    js_set_encode(cookie,JS_US_ASCII);
    if((key = js_create(256,1)) == 0)
        softerror("Can not allocate key string in gencookie");
    js_set_encode(key,JS_US_ASCII);

    /* Get and set up the encryption key */
    js_qstr2js(tstr,"kiwi_key");
    read_kvar(tstr,key);
    kjs_key_setup(key);

    if(crypto == 0) { /* Get cookie to encrypt from user input */
        js_getline_stdin(cookie);
        if(js_length(cookie) != COOKIE_SIZE + 1) /* We have a newline! */
            harderror("Encrypted cookie must be seven characters long");
        }
    else { /* Get it from crypto argument */
        js_qstr2js(cookie,crypto);
        if(js_length(cookie) != COOKIE_SIZE)
            harderror("Encrypted cookie must be seven characters long");
        }

    data = kjs_parse_cookie(cookie);
    data = kjs_grokdata(data,format);
    printf("%s","Data format: ");
    js_show_stdout(format);
    printf("\n");
   
    /* If the message is a timestamp, tell them what the time stamp is in
       both UNIX and English time */
    js_qstr2js(tstr,"TIMEOUT");
    tnum = js_fgrep(tstr,format);
    if(tnum == JS_ERROR)
        softerror("Strange problem determining format");
    if(tnum != -2) {
        /* Cut and paste from Kiwi version 1 */
        printf("Message data: %d, or %s",data,ctime((time_t *)&data));
	return;
        }
   
    /* If the message is an 16-IP range, tell them what IP range it is */
    js_qstr2js(tstr,"28BIT IP BLOCK");
    tnum = js_fgrep(tstr,format);
    if(tnum == JS_ERROR)
        softerror("Strange problem determining format.");
    if(tnum == 0) {
        printf("Message data: %s/28\n",data_to_ip(data,string));
        return;
	}

    /* If the message is a five-letter embedded message (for mailing lists and 
       other permanent email addresses), tell them what the five letters are */
    js_qstr2js(tstr,"FIVE LETTERS");
    tnum = js_fgrep(tstr,format);
    if(tnum == JS_ERROR)
        softerror("A Strange problem determining format");
    if(tnum == 0) {
        /* Cut and paste from Kiwi version 1 */
	/* With some changes to accomindate UTF8 output */
        unsigned char mesg[6];
	data_to_five(data,mesg);

	/* Ugly hacky code to see if we need to utf8-ize the output */
	js_qstr2js(tstr,"kiwi_utf8_output");
	*(key->string) = 0;
	read_kvar(tstr,key);
	if(*(key->string) == 'T') { /* Then we need to utf8ize the mesg 
	                               string */
	    for(tnum=0;tnum<5;tnum++) {
	        if(mesg[tnum] >=0xe1 && mesg[tnum] <=0xfa) {
		    printf("%c",0xc3);
		    mesg[tnum] -= 0x40;
		    }
                printf("%c",mesg[tnum]);
	        }
            printf("\n");
            }
        else { /* We don't need to utf8ize the output */
	    printf("Message: %s\n",mesg);
	    }
	}

    }
 
/* Generate a Kiwi cookies, and output them to the standard output.
   input: the ip/5-letter code we wish to encrypt (NULL if timestamp crypted)
   output: none (outputs information to stdout)
   global vars used: But only in kead_kvar that this calls
*/
void gencookie(char *tocrypt) {
    /* Set up the key for encrypting data */
    js_string *tstr, *key;
    Kiwi_cdata data;
    time_t time_s;

    if((tstr = js_create(256,1)) == 0)
        softerror("Can not allocate temp string in gencookie");
    js_set_encode(tstr,JS_US_ASCII);
    if((key = js_create(256,1)) == 0)
        softerror("Can not allocate key string in gencookie");
    js_set_encode(key,JS_US_ASCII);

    /* Get and set up the encryption key */
    js_qstr2js(tstr,"kiwi_key");
    read_kvar(tstr,key);
    kjs_key_setup(key);

    if(tocrypt == 0) { /* Timestamp encryption */ 
        /* Get the time stamp and make cookies for short, mid, and long 
           timeout based on it */
        time_s = time(0);

        /* Short timeout */
        data = 0x80000000;
        data |= (time_s & 0xFFFFFFFF) >> 4; 
        kjs_make_cookie(data,tstr);
        printf("%s","Cookie with short timeout: ");
        js_show_stdout(tstr);
        printf("\n");

        /* Mid timeout */
        data = 0x90000000;
        data |= (time_s & 0xFFFFFFFF) >> 4; 
        kjs_make_cookie(data,tstr);
        printf("%s","Cookie with medium timeout (Usenet cookie): ");
        js_show_stdout(tstr);
        printf("\n");
 
        /* Long timeout */
        data = 0xA0000000;
        data |= (time_s & 0xFFFFFFFF) >> 4; 
        kjs_make_cookie(data,tstr);
        printf("%s","Cookie with long timeout: ");
        js_show_stdout(tstr);
        printf("\n");

        return;
        }
    else { /* Argument was given to tocrypt */

        if(js_qstr2js(tstr,tocrypt) == JS_ERROR)
	    harderror("Error converting REMOTE_ADDR/argv[1]");

        if(js_length(tstr) > 6 && *(tstr->string) >= '0' 
	   && *(tstr->string) <= '9') /* IP to crypt */ {
            int place = 0; /* Place to look for an octet */
            u_int8_t ipo[4];
            int counter;
	    js_string *dotq;
	    
	    if((dotq = js_create(256,1)) == 0)
	        softerror("Can not allocate memory for dotq");
           
	    js_set_encode(dotq,JS_US_ASCII);
	    js_qstr2js(dotq,".");

            for(counter = 0;counter < 4;counter++) {
                if(place == -2) 
		    harderror("IP must be in dotted octet form like 10.1.1.1");
                else {
                    if(place != 0)
                        place++;
	            ipo[counter] = js_atoi(tstr,place);
                    }
		place = js_match_offset(dotq,tstr,place);
		if(place == JS_ERROR)
		    harderror("Problem matching on IP string");
                }
            
	    data = (ipo[0] << 24) | (ipo[1] << 16) | (ipo[2] << 8) | ipo[3];
	    data >>= 4;
	    data |= 0xC0000000;
	    kjs_make_cookie(data,tstr);
	    js_show_stdout(tstr); 
	    return;
	    }
        else { /* 5-letter message to crypt */
	    int val,counter,protect,offset;
            data = 0;

            if(js_qstr2js(tstr,tocrypt) == JS_ERROR)
	        harderror("Unable to js-ize 5-letter message!");

            if(js_length(tstr) < 5 || js_length(tstr) > 10)
	        harderror("Message must be exactly 5 letters long.");

	    offset = 0;
            for(counter=0;counter<5;counter++) {
	        data <<= 5;
	        /* We do it this way because a return code of 64 indicates
		   that we got the first letter in a utf-8 sequence */
                protect = 0; 
	        do {

		    val = js_val(tstr,offset++);
                    if(val == JS_ERROR)
                        harderror("Problem getting value of message char");
             
		    val = five_letter_unmap(val);
                    protect++; 
		    } while(val == 64 && protect < 2);

                if(val == JS_ERROR || val < 0 || val >= 32)
                    harderror("Please use only lower case letters in message");

                data |= val & 31;
                }

	    data |= 0x7C000000;
            kjs_make_cookie(data,tstr);
	    js_show_stdout(tstr);
	    return;
            }
        } 

    /* Something wrong if we get here */
    harderror("Invalid data given to clicrypt!");
    }

/* Sendmail wrapper for outgoing mail */
void wrapper(int argc, char **argv) {
    /* We need to know three things from .kiwirc: 
    
    1. What MTA we are running? (Qmail or Sendmail) 

    2. Our Kiwi key.

    3. The location of sendmail

    */

    js_string *qmailp, *key, *sendmail, *tstr, *cookie;

    Kiwi_cdata data;

    int stream[2];

    /* Initialize the strings */
    if((qmailp = js_create(256,1)) == 0)
        softerror("Can not allocate qmailp string in wrapper");
    js_set_encode(qmailp,JS_US_ASCII);
    if((key = js_create(256,1)) == 0)
        softerror("Can not allocate key string in wrapper");
    js_set_encode(key,JS_US_ASCII);
    if((sendmail = js_create(256,1)) == 0)
        softerror("Can not allocate sendmail path string in wrapper");
    js_set_encode(sendmail,JS_US_ASCII);
    if((tstr = js_create(256,1)) == 0)
        softerror("Can not allocate tstr string in wrapper");
    js_set_encode(tstr,JS_US_ASCII);
    if((cookie = js_create(256,1)) == 0)
        softerror("Can not allocate cookie string in wrapper");
    js_set_encode(cookie,JS_US_ASCII);

    /* Obtain the values from the read .kiwirc file */
    js_qstr2js(tstr,"kiwi_qmail");
    read_kvar(tstr,qmailp);
    js_qstr2js(tstr,"kiwi_key");
    read_kvar(tstr,key);
    js_qstr2js(tstr,"kiwi_sendmail_path");
    read_kvar(tstr,sendmail);

    /* Generate the cookie to add to the email */
    data = 0xA0000000;
    data |= (time(0) & 0xFFFFFFFF) >> 4; 
    kjs_key_setup(key);
    if(kjs_make_cookie(data,cookie) == JS_ERROR)
        softerror("Problem setting up encrypted timestamp cookie");

    /* Set up an environmental variable to pass to the child */
    setenv("KIWI_STATUS","OUTGOING",1);

    /* Lots of cut and paste from Kiwi1 here, adapted to Kiwi2 */
    /* Set up things to pipe stdout to the sendmail child process */
    if(pipe(stream) != 0)
        softerror("Unable to initialize pipe");
   
    if(fork()) {
        /* Parent */
	char sendmail_cmd[256];
	close(stream[1]); /* close the output stream */
	dup2(stream[0],0); /* duplicate the standard input */
        if(js_js2str(sendmail,sendmail_cmd,200) == JS_ERROR)
	    harderror("Can not convert sendmail command to null-t string");
        execv(sendmail_cmd,argv);
	/* If we get here, there is a problem */
	harderror("Can not start sendmail!");
	}
    else { 
        /* Child */

	js_string *line, *tstr2, *lower;
	int inheader = 1;
        int addrline = 0;
	int showline, mungeline, match;

        if((line = js_create(256,1)) == 0)
            softerror("Can not allocate line string in wrapper");
        js_set_encode(line,JS_US_ASCII);
        if((tstr2 = js_create(256,1)) == 0)
            softerror("Can not allocate tstr2 string in wrapper");
        js_set_encode(tstr2,JS_US_ASCII);
        if((lower = js_create(256,1)) == 0)
            softerror("Can not allocate lower string in wrapper");
        js_set_encode(lower,JS_US_ASCII);
	
	close(stream[0]); /* Close the input stream */
	dup2(stream[1],1); /* duplicate the standard output to the pipe.
			      In other words, the standard output now goes
			      in to the pipe (sendmail) instead of to the
			      tty of the process */

        /* First, give return-path value so it doesn't
           have a real email address */
        js_qstr2js(tstr,"kiwi_return_path");
        read_kvar(tstr,tstr2);
        if(js_length(tstr2) == 0)
            js_qstr2js(tstr2,"nobody@example.com");
        js_qstr2js(tstr,"Return-Path: ");
        js_append(tstr2,tstr);
        js_qstr2js(tstr2,"\n");
        js_append(tstr2,tstr);
        js_show_stdout(tstr);

        mungeline = showline = 1;
        do {
            
	    if(js_getline_stdin(line) == JS_ERROR)
	        harderror("Problem reading line from stdin");

            if(feof(stdin))
                break;

	    js_newline_chars(tstr);
	    js_space_chars(tstr2);
	    js_append(tstr2,tstr);
	    if(js_notmatch(tstr,line) == -2) /* If this is a blank line */
	        inheader = 0;

            js_qstr2js(tstr,AN); /* Alphanumeric characters */
            if(js_match(tstr,line) == 0) 
                mungeline = showline = 1;

	    /* Make lower a lower-case version of line */
            js_copy(line,lower);
	    js_tolower(lower);

	    /* Various lines in the heaqder we do not munge */
	    js_qstr2js(tstr,"message-id:");
	    if(js_fgrep(tstr,lower) == 0)
		mungeline = 0;
	    js_qstr2js(tstr,"to:");
	    if(js_fgrep(tstr,lower) == 0)
		mungeline = 0;
	    js_qstr2js(tstr,"cc:");
	    if(js_fgrep(tstr,lower) == 0)
		mungeline = 0;
	    js_qstr2js(tstr,"bcc:");
	    if(js_fgrep(tstr,lower) == 0)
		mungeline = 0;
	    js_qstr2js(tstr,"content-");
	    if(js_fgrep(tstr,lower) == 0)
		mungeline = 0;
	    js_qstr2js(tstr,"in-reply-to:");
	    if(js_fgrep(tstr,lower) == 0)
		mungeline = 0;
	    js_qstr2js(tstr,"subject:");
	    if(js_fgrep(tstr,lower) == 0)
		mungeline = 0;
            /* We do not show an X-Sender: line */	
            js_qstr2js(tstr,"x-sender:"); 
            if(js_fgrep(tstr,lower) == 0)
		showline = 0;

            if(mungeline && inheader) {
	    	js_qstr2js(tstr,"@");
	        match = js_match(tstr,line);
		if(match == JS_ERROR)
		    harderror("Problem running match against line");
	
		if(match != -2) {
		    /* Inset actual Kiwi cookie */
		    if(js_insert(cookie,line,match) == JS_ERROR)
		        harderror("Problem inserting Kiwi cookie");

		    /* Then inset -/+ (Done this way because insert inserts
                       _after_ the string place in question) */
		    js_qstr2js(tstr,"true");
		    js_tolower(qmailp);
		    if(js_fgrep(tstr,qmailp) == 0) {
		        js_qstr2js(tstr,"-");
			if(js_insert(tstr,line,match) == JS_ERROR)
			    harderror("Problem inserting qmail dash");
                        }
                    else {
		        js_qstr2js(tstr,"+");
			if(js_insert(tstr,line,match) == JS_ERROR)
			    harderror("Problem inserting sendmail dash");
                        }
		    }
		}  

	    if(showline) 
	        js_show_stdout(line);

	    } while(!feof(stdin));
	}
    /* Both parent and child */
    exit(0); 
    }

/* Getaddress: Get an email address from a mail header line
   Input: Pointer to line, pointer to js_string to place address in
   Output: JS_ERROR on error, JS_SUCCESS on success 
   (address will be null if address not found) */
int getaddress(js_string *line, js_string *address) {

    js_string *exp, *tstr;
    int begin, end, place;

    /* Sanitry checks */
    if(kiwi_goodjs(line) == JS_ERROR)
        return JS_ERROR;
    if(kiwi_goodjs(address) == JS_ERROR)
        return JS_ERROR;

    /* Blank out address */
    js_qstr2js(address,"");

    /* Allocate the expression we match against */
    if((exp = js_create(256,1)) == 0)
        return JS_ERROR;
    js_set_encode(exp,JS_US_ASCII);
    /* And a temporary string */
    if((tstr = js_create(256,1)) == 0)
        return JS_ERROR;
    js_set_encode(tstr,JS_US_ASCII);

    /* Look for anything with an @ in it between <> */
    if(js_qstr2js(exp,"<") == JS_ERROR)
        return JS_ERROR;
    if((begin = js_fgrep(exp,line)) == JS_ERROR)
        return JS_ERROR;
    if(begin != -2) { /* < found */
        if(js_qstr2js(exp,">") == JS_ERROR)
	    return JS_ERROR;
        if((end = js_fgrep_offset(exp,line,begin)) == JS_ERROR)
	    return JS_ERROR;
        if(end != -2) { /* > found after < */
	    if((js_substr(line,address,begin + 1,end - begin - 1)) == JS_ERROR)
	        return JS_ERROR;
            if(js_qstr2js(exp,"@") == JS_ERROR)
	        return JS_ERROR;
            if((place = js_fgrep(exp,address)) == JS_ERROR)
	        return JS_ERROR;
            if(place != -2) /* @ in address */
	        return JS_SUCCESS;
            }
        }

    /* If <> search fails, look for bare word with an @ in it */
    if(js_qstr2js(exp,"@") == JS_ERROR)
        return JS_ERROR;
    if((place = js_fgrep(exp,line)) == JS_ERROR)
        return JS_ERROR;
    if(place == -2) /* No @ found */
        return JS_SUCCESS; /* Address string is 0-length string */

    /* Start looking for whitespace/newlines */
    js_space_chars(tstr);
    js_newline_chars(exp);
    js_append(tstr,exp);
    end = -1; /* Doesn't mean error--this makes code elegant */
    while(end < place) {
        begin = end;
	if((end = js_match_offset(exp,line,begin + 1)) == JS_ERROR)
	    return JS_ERROR;
        if(end == -2) { /* @ in word at end of string */ 
	    end = js_length(line) + 1;
	    break;
	    }
        }

    /* The word is between the last whitespace before the @ (or the
       beginning of the string) and the first whitespace after the @ 
       (or the end of the string) */
    if((js_substr(line,address,begin + 1,end - begin - 1)) == JS_ERROR)
        return JS_ERROR;
    return JS_SUCCESS;
    }

/* parseaddress: Parse the kiwi-cookie in an email address 
   input: email address to parse
   output: data value of cookie in address
   global vars used: kiwi key needs to be previously set up with kjs_key_setup
                     We use the kiwi_qmail and kiwi_password kvars
*/
Kiwi_cdata parseaddress(js_string *address) {
    /* We need to use the following kvars (symbols defined when 
                                           parsing .kiwirc)

       kiwi_qmail: if we are using Qmail or sendmail

       kiwi_password: if we have a valid kiwi password */

    js_string *qmailp, *password, *tstr, *exp;

    int dash, at, is_qmail = 0, temp;

    Kiwi_cdata data;

    /* Initialize the strings */
    if((qmailp = js_create(256,1)) == 0)
        softerror("Can not allocate qmailp string in wrapper");
    js_set_encode(qmailp,JS_US_ASCII);
    if((password = js_create(256,1)) == 0)
        softerror("Can not allocate key string in wrapper");
    js_set_encode(password,JS_US_ASCII);
    if((tstr = js_create(256,1)) == 0)
        softerror("Can not allocate key string in wrapper");
    js_set_encode(tstr,JS_US_ASCII);
    if((exp = js_create(256,1)) == 0)
        softerror("Can not allocate key string in wrapper");
    js_set_encode(exp,JS_US_ASCII);

    /* Obtain the values from the read .kiwirc file */
    js_qstr2js(tstr,"kiwi_qmail");
    read_kvar(tstr,qmailp);
    js_qstr2js(tstr,"kiwi_password");
    read_kvar(tstr,password);

    /* Find the "@" and the last dash before the "@" */
    js_qstr2js(exp,"@");
    at = js_fgrep(exp,address);
    if(at == JS_ERROR)
        return 0;
    if(at == -2)
        return 1; /* No cookie found -- invalid */
    /* See if we are running Qmail or Sendmail */
    js_qstr2js(exp,"true");
    js_copy(qmailp,tstr);
    js_tolower(tstr);
    if(js_fgrep(exp,tstr) == 0)
        is_qmail = 1;
    /* Now, look for the dash */
    if(is_qmail)
        js_qstr2js(exp,"-");
    else
        js_qstr2js(exp,"+");
    temp = 0;
    dash = -2;
    while(temp < at && temp != -2) {
        dash = temp;
	if(temp < js_length(address) - 2)
            temp = js_fgrep_offset(exp,address,temp + 1);
        else
	    temp = -2;
        if(temp == JS_ERROR)
	    return 0;
        }
    if(dash == -2) /* No dash found before at */
        return 1; /* No cookie -- invalid */
    if(at - dash < 2) /* invalid values */
        return 1;
    /* Make tstr the cookie we want to grok */
    if(js_substr(address,tstr,dash + 1,at - dash - 1) == JS_ERROR)
        return 0;
    /* Password check. Todo: Make case-insensitive */
    if(js_issame(tstr,password) == 1) 
        return 2;
    data = kjs_parse_cookie(tstr);
    if(data == 0 || data == 1 || data == 2)
        return 1;
    return data; 
    }

/* Mail filter for incoming mail */
void infilter(int argc, char **argv) {

    /* This is how the input filter functions:

       We have a very large js_string object (32k) which stores the header.

       If kiwi_qmail is defined as TRUE (case-insensitive) in .kiwirc, we 
       look for the address this mail was delivered to _only_ in
       the environmental variable DTLINE.

       Until the header ends, we read every line of the mail header.

       If we are using sendmail, and the line begins with '<TAB>for',
       we look for the address the message was delivered to there.

       If the line begins "From:", and the return address isn't defined, we 
       get the return address from the line (using getaddress)

       If the line begins reply-to, we get the return address from the line.

       A header can have only one from and one reply-to header.  The infilter
       will throw an exception if we have multiple Froms or multiple 
       reply-to headers.

       We use parseaddress to get the value of the cookie in the address this
       message was delivered to.  Depending on the value of the cookie:

       1. The cookie is invalid (0 or 1): If kiwi_bouncemail is defined,
          then we use the file pointed to by kiwi_bouncemail as a template
	  to send a message to the return address found above.  If 
	  kiwi_bouncemail is _not_ defined, we silently discard the mail.
	  If either case, we log that the message was rejected.

       2. If the cookie indicates a correct password (2): We perform the
          default mail delivery on the message.

       3. Otherwise, we run kjs_grokdata on the value of the decoded cookie.

       Depending on the output of kjs_grokdata:
 
       1. If the data type is "MID TIMEOUT", and the data value 
          is between one week in the future and kiwi_days_mid days in 
	  the past, then we act as follows:  If kiwi_mid_process is 
	  defined, we pipe the mail in to the program that 
	  kiwi_mid_process points to.  If kiwi_mid_process is not 
	  defined, we perform the default mail delivery.

       2. If the data type is "SHORT TIMEOUT", and the data value of
          is between one week in the future and kiwi_days_short days
          in the past, we perform the default mail delivery.

       3. If the data type is "LONG TIMEOUT", and the data value 
          is between one week in the future and kiwi_days_long days
          in the past, we perform the default mail delivery.

       4. If the data type is "28BIT IP BLOCK" or "FIVE LETTERS",
          we perform the default mail delivery.

       If we are to perform the default mail delivery:

       1. If kiwi_forward is "TRUE" (case insensitive), then pipe the
          mail to the kiwi_sendmail_path program, with the environmental
	  variable "KIWI_STATUS" to "INCOMING", the environmental 
	  variable "KIWI_DATATYPE" to the data type of the decoded cookie,
	  and the env variable "KIWI_DATA" to the data in the decoded
	  cookie (itoa if SHORT/MID/LONG timeout, "10.1.2.3/28" form if
	  ip block, and "abcde" form if five letter message).  The 
	  kiwi_sendmail_path program is given the kiwi_my_address
	  value as the one single argument to kiwi_sendmail_path.

       2. If kiwi_append is "TRUE" (case insensitive), then perform
          an exclusive flock on the file pointed to with kiwi_my_mailbox,
	  open the file for appending.  Add a "From nobody@nowhere date"-type
	  line if kiwi_qmail is "TRUE" (any case).  Next, append the
	  mail message to the file in question.  Unlock the file before 
	  closing the file and exiting.
       
       If the default delivery was _not_ performed:

       If kiwi_bouncemail is defined, then we use the file pointed to 
       by kiwi_bouncemail as a template to send a message to the return 
       address found above.  If kiwi_bouncemail is _not_ defined, we 
       silently discard the mail.  If either case, we log that the 
       message was rejected.

    */

    /* We use the following kvars (symbols in .kiwirc):

       kiwi_qmail
       kiwi_key
       kiwi_bouncemail
       kiwi_days_short
       kiwi_days_mid
       kiwi_days_long
       kiwi_forward
       kiwi_sendmail_path
       kiwi_my_address
       kiwi_append
       kiwi_my_mailbox

     */

    /* Declare variables */
    js_string *qmail, *key, *shortstr, *midstr, *longstr,
              *forward, *sendmail, *address, *append, *mailbox;
    js_string *tstr, *tstr2, *header, *lower, *deliv, *line, *from;
    js_string *datatype, *midprocess, *password;

    /* Booleans */
    int is_qmail = 0;
    int seen_from = 0;
    int seen_replyto = 0;
    int seen_deliv = 0;
    int inheader = 1; 

    /* NULL-terminated strings */
    char *nult_str;
    char strn[256];

    /* Integers */
    int tnum;
    
    /* Decoded cookie */
    Kiwi_cdata data;

    /* Time stamp */
    time_t now;

    /* Allocate memory for the strings */
    if((key = js_create(256,1)) == 0)
        softerror("Can not allocate key string in infilter");
    if((tstr = js_create(256,1)) == 0)
        softerror("Can not allocate tstr string in infilter");
    if((tstr2 = js_create(256,1)) == 0)
        softerror("Can not allocate tstr2 string in infilter");
    if((qmail = js_create(256,1)) == 0)
        softerror("Unable to create qmail in infilter");
    if((shortstr = js_create(256,1)) == 0)
        softerror("Unable to create shortstr in infilter");
    if((midstr = js_create(256,1)) == 0)
        softerror("Unable to create midstr in infilter");
    if((longstr = js_create(256,1)) == 0)
        softerror("Unable to create longstr in infilter");
    if((forward = js_create(256,1)) == 0)
        softerror("Unable to create forward in infilter");
    if((sendmail = js_create(256,1)) == 0)
        softerror("Unable to create sendmail in infilter");
    if((address = js_create(256,1)) == 0)
        softerror("Unable to create address in infilter");
    if((append = js_create(256,1)) == 0)
        softerror("Unable to create append in infilter");
    if((mailbox = js_create(256,1)) == 0)
        softerror("Unable to create mailbox in infilter");
    if((lower = js_create(256,1)) == 0)
        softerror("Unable to create lower in infilter");
    if((deliv = js_create(256,1)) == 0)
        softerror("Unable to create lower in infilter");
    if((line = js_create(256,1)) == 0)
        softerror("Unable to create line in infilter");
    if((from = js_create(256,1)) == 0)
        softerror("Unable to create from in infilter");
    if((datatype = js_create(256,1)) == 0)
        softerror("Unable to create datatype in infilter");
    if((midprocess = js_create(256,1)) == 0)
        softerror("Unable to create midprocess in infilter");
    /* The header is a lot larger than the other strings */
    if((header = js_create(32768,1)) == 0)
        softerror("Unable to allocate memory to store mail header");

    /* Set the encoding for the strings */
    js_set_encode(key,JS_US_ASCII);
    js_set_encode(tstr,JS_US_ASCII);
    js_set_encode(qmail,JS_US_ASCII);
    js_set_encode(shortstr,JS_US_ASCII);
    js_set_encode(midstr,JS_US_ASCII);
    js_set_encode(longstr,JS_US_ASCII);
    js_set_encode(forward,JS_US_ASCII);
    js_set_encode(sendmail,JS_US_ASCII);
    js_set_encode(address,JS_US_ASCII);
    js_set_encode(append,JS_US_ASCII);
    js_set_encode(mailbox,JS_US_ASCII);
    js_set_encode(lower,JS_US_ASCII);
    js_set_encode(deliv,JS_US_ASCII);
    js_set_encode(line,JS_US_ASCII);
    js_set_encode(from,JS_US_ASCII);
    js_set_encode(datatype,JS_US_ASCII);
    js_set_encode(midprocess,JS_US_ASCII);
    js_set_encode(header,JS_US_ASCII);

    /* Grab the values for the strings from the kvars */
    js_qstr2js(tstr,"kiwi_qmail");
    read_kvar(tstr,qmail);
    js_qstr2js(tstr,"kiwi_days_short");
    read_kvar(tstr,shortstr);
    js_qstr2js(tstr,"kiwi_days_mid");
    read_kvar(tstr,midstr);
    js_qstr2js(tstr,"kiwi_days_long");
    read_kvar(tstr,longstr);
    js_qstr2js(tstr,"kiwi_forward");
    read_kvar(tstr,forward);
    js_qstr2js(tstr,"kiwi_sendmail_path");
    read_kvar(tstr,sendmail);
    js_qstr2js(tstr,"kiwi_my_address");
    read_kvar(tstr,address);
    js_qstr2js(tstr,"kiwi_append");
    read_kvar(tstr,append);
    js_qstr2js(tstr,"kiwi_my_mailbox");
    read_kvar(tstr,mailbox);
    js_qstr2js(tstr,"kiwi_key");
    read_kvar(tstr,key); 

    /* Set up the encryption key */
    kjs_key_setup(key);

    /* Determine if we are running qmail */
    js_copy(qmail,lower);
    js_tolower(lower);
    js_qstr2js(tstr,"true");
    if(js_issame(lower,tstr)) 
        is_qmail = 1;

    /* If running Qmail, get the deliv address from DTLINE */
    if(is_qmail) {
        nult_str = getenv("DTLINE");
	if(nult_str == NULL)
	    harderror("You claim to run Qmail, but DTLINE isn't defined!");
        if(js_qstr2js(deliv,nult_str) == JS_ERROR)
	    harderror("Can not get env variable DTLINE");
        js_tolower(deliv);
        seen_deliv = 1;
	}

    /* Process data from the mail header */
    while(inheader && !feof(stdin)) {
        if(js_getline_stdin(line) == JS_ERROR)
	    harderror("Error reading line from standard input");
        if(feof(stdin))
	    break;

        /* If this is a blank line, end the header */
	js_newline_chars(tstr);
	js_space_chars(tstr2);
	js_append(tstr2,tstr);
	if(js_notmatch(tstr,line) == -2)
	    inheader = 0;

        /* If we are using Sendmail, then process a /^\tfor/ line */
	if(!is_qmail) {
	    js_qstr2js(tstr,"\tfor");
            tnum = js_fgrep(tstr,line);
	    if(tnum == JS_ERROR)
	        harderror("Problem running fgrep on line");
            if(tnum == 0)
	        if(js_copy(line,deliv) == JS_ERROR)
		    harderror("Error copying deliv line");
	    }

        /* Process a /^From/ line */
	js_qstr2js(tstr,"from: ");
	js_copy(line,lower);
	js_tolower(lower);
	tnum = js_fgrep(tstr,lower);
	if(tnum == JS_ERROR)
	    harderror("Problem running Fgrep on line");
        if(tnum == 0) {
	    if(seen_from) 
	        harderror("Only one From line in header allowed");
	    seen_from = 1;
	    if(!seen_replyto) 
	        getaddress(line,from);
	    }

        /* Process a /^Reply-To:/ line */
	js_qstr2js(tstr,"reply-to: ");
	js_copy(line,lower);
	js_tolower(lower);
	tnum = js_fgrep(tstr,lower);
	if(tnum == JS_ERROR)
	    harderror("Problem running fgrep on line.");
        if(tnum == 0) {
	    if(seen_replyto) 
	        harderror("Only one Reply-To line in header allowed");
	    seen_replyto = 1;
	    getaddress(line,from);
	    }

	if(js_append(line,header) == JS_ERROR)
	    harderror("Your header is too big");
        }

    
    /* Decode the cookie in the from variable */
    data = parseaddress(deliv);
    if(data == 0) 
        harderror("Fatal error parsing email address, exiting");
  
    /* Set the environemtal variable KIWI_FROM to the address this email is
       from */ 
    if(js_length(from) != 0) {
        if(js_js2str(from,strn,200) == JS_ERROR)
            harderror("Email address is too long");
        setenv("KIWI_FROM",strn,1);
        }

    setenv("KIWI_STATUS","INCOMING",1); 

    /* Handle the case of no from address */
    if(!seen_from && !seen_replyto || js_length(from)==0) 
        js_qstr2js(from,"(Address not defined)");

    if(data == 1) { /* Bad parity, usually */
	setenv("KIWI_DATATYPE","BAD PARITY",1);
	setenv("KIWI_DATA","N/A",1);
        bounce(from,header,deliv);
	}
    
    if(data == 2) { /* Good password */
	setenv("KIWI_DATATYPE","GOOD PASSWORD",1);

        /* Set the KIWI_DATA to the good password */
        if((password = js_create(256,1)) == 0)
            softerror("Unable to allocate password string");
        js_set_encode(password,JS_US_ASCII);
        js_qstr2js(tstr,"kiwi_password");
        read_kvar(tstr,password); 
        if(js_js2str(password,strn,200) == JS_ERROR)
            softerror("Problem converting kiwi password to null-term string");
	setenv("KIWI_DATA",strn,1);

        deliver(header);
	}

    data = kjs_grokdata(data,datatype); 

    /* Set the the KIWI_DATATYPE environmental variable */
    if(js_js2str(datatype,strn,200) == JS_ERROR)
        softerror("Problem converting kiwi datatype to null-term string");
    setenv("KIWI_DATATYPE",strn,1);

    /* Deliver on "28BIT IP BLOCK" */
    js_qstr2js(tstr,"28BIT IP BLOCK");
    tnum = js_fgrep(tstr,datatype);
    if(tnum == JS_ERROR)
        softerror("Strange error when determining datatype");
    if(tnum == 0) {
        setenv("KIWI_DATA",data_to_ip(data,strn),1); 
        deliver(header);
	}

    /* Deliver on "FIVE LETTERS" */
    js_qstr2js(tstr,"FIVE LETTERS");
    tnum = js_fgrep(tstr,datatype);
    if(tnum == JS_ERROR)
        softerror("Strange error when determining datatype.");
    if(tnum == 0) {
        setenv("KIWI_DATA",data_to_five(data,strn),1);
        deliver(header);
	}

    /* Deliver within time frame on "SHORT TIMEOUT" */
    js_qstr2js(tstr,"SHORT TIMEOUT");
    tnum = js_fgrep(tstr,datatype);
    if(tnum == JS_ERROR)
        softerror("Strange error when determining datatype");
    if(tnum == 0) {
        snprintf(strn,100,"%d",data);
        setenv("KIWI_DATA",strn,1);
        now = time(0);
        tnum = js_atoi(shortstr,0);
        tnum *= 86400; /* Number of seconds in a day */
        if(data > now - tnum && data < now + (86400 * 7))
	    deliver(header);
        else
	    bounce(from,header,deliv);
        } 	

    /* Deliver within time frame on "LONG TIMEOUT" */
    js_qstr2js(tstr,"LONG TIMEOUT");
    tnum = js_fgrep(tstr,datatype);
    if(tnum == JS_ERROR)
        softerror("Strange error when determining datatype");
    if(tnum == 0) {
        snprintf(strn,100,"%d",data);
        setenv("KIWI_DATA",strn,1);
        now = time(0);
        tnum = js_atoi(longstr,0);
        tnum *= 86400; /* Number of seconds in a day */
        if(data > now - tnum && data < now + (86400 * 7))
	    deliver(header);
        else
	    bounce(from,header,deliv);
        } 

    /* Handle "MID TIMEOUT" */
    js_qstr2js(tstr,"MID TIMEOUT");
    tnum = js_fgrep(tstr,datatype);
    if(tnum == JS_ERROR)
        softerror("Strange error when determining datatype");
    if(tnum == 0) {
        snprintf(strn,100,"%d",data);
        setenv("KIWI_DATA",strn,1);
        now = time(0);
        tnum = js_atoi(midstr,0);
        tnum *= 86400; /* Number of seconds in a day */
        if(data > now - tnum && data < now + (86400 * 7)) {
            js_qstr2js(tstr,"kiwi_mid_process");
            read_kvar(tstr,midprocess); 
            if(js_length(midprocess) == 0)
	        deliver(header);
            else { /* Adapted cut and paste from deliver */
                 /* Sanity check */
	         js_qstr2js(tstr,"@");
	         if(js_fgrep(tstr,address) <= 0)
	             softerror("kiwi_my_address address needs an @ in it");
                 /* Pipe the mail throught kiwi_mid_process */
                 piper(header,address,midprocess);
                 } 
	    }
        else
	    bounce(from,header,deliv);
        } 

    /* Unknown data types get a bounce message */
    bounce(from,header,deliv);
    }

/* Bounce: Process a message that is not to be delivered
   input: from: address to send bounce to
          header: Header of message to bounce
   output: Void function
*/
void bounce(js_string *from, js_string *header, js_string *deliv) {
    js_string *bouncemail, *maillog, *tstr, *message;
    js_string *boing;
    js_file *desc;
    char strn[256];
    time_t now;

    /* Reset Kiwi status to "bouncing" */
    setenv("KIWI_STATUS","BOUNCING",1); 

    /* Set up variables */
    desc = js_alloc(1,sizeof(js_file));
    if(desc == 0)
        softerror("Unable to allocate memory for file in bounce");
    if((maillog = js_create(256,1)) == 0)
        softerror("Unable to create maillog in bounce");
    js_set_encode(maillog,JS_US_ASCII);
    if((bouncemail = js_create(256,1)) == 0)
        softerror("Unable to create bouncemail in bounce");
    js_set_encode(bouncemail,JS_US_ASCII);
    if((tstr = js_create(256,1)) == 0)
        softerror("Unable to create tstr in bounce");
    js_set_encode(tstr,JS_US_ASCII);
    if((message = js_create(256,1)) == 0)
        softerror("Unable to create message in bounce");
    js_set_encode(message,JS_US_ASCII);

    /* Get variables that determine if we send a bounce message */
    js_qstr2js(tstr,"kiwi_bouncemail");
    read_kvar(tstr,bouncemail);
    js_qstr2js(tstr,"kiwi_maillog");
    read_kvar(tstr,maillog);

    /* Make a note that the mail has been rejected */
    if(js_length(maillog) != 0) {
        if(js_open_append(maillog,desc) == JS_ERROR)
	    softerror("Unable to open logfile in bounce");
        js_qstr2js(message,"Mail from "); 
	js_append(from,message);
        js_qstr2js(tstr," sent to ");
        js_append(tstr,message);
        getaddress(deliv,tstr);
        js_append(tstr,message);
	js_qstr2js(tstr," rejected ");
        js_append(tstr,message);
        time(&now);
        js_qstr2js(tstr,ctime(&now));
        js_append(tstr,message); 
	js_lock(desc);
	js_write(desc,message);
	js_unlock(desc);
	js_close(desc);
        }

    /* If bouncemail is set, then send out a bounce message */
    if(js_length(bouncemail) != 0) {
	if(js_open_read(bouncemail,desc) == JS_ERROR)
            softerror("Unable to read bounce message");
	if((boing = js_create(45000,1)) == 0)
	    softerror("Can not allocate memory for header in bounce");
        js_set_encode(boing,JS_US_ASCII);
	js_read(desc,boing,45000);
	if(js_append(header,boing) == JS_ERROR)
	    harderror("Header + Bounce mail size too big");
        forward(boing,from);
	exit(0); /* We should never get here */
        }

    exit(0);
    }

/* Deliver: Deliver a message that has a valid Kiwi cookie (or password)
   input: header: header of message we are delivering
   output: Void function
*/

void deliver(js_string *header) {
    js_string *do_forward, *do_append, *forwarder, *appender, *inprocess;
    js_string *tstr, *lower;

    /* Create the strings */
    if((do_forward = js_create(256,1)) == 0)
        softerror("Unable to create do_forward in delive");
    js_set_encode(do_forward,JS_US_ASCII);
    if((do_append = js_create(256,1)) == 0)
        softerror("Unable to create do_append in delive");
    js_set_encode(do_append,JS_US_ASCII);
    if((forwarder = js_create(256,1)) == 0)
        softerror("Unable to create forwarder in delive");
    js_set_encode(forwarder,JS_US_ASCII);
    if((appender = js_create(256,1)) == 0)
        softerror("Unable to create appender in delive");
    js_set_encode(appender,JS_US_ASCII);
    if((tstr = js_create(256,1)) == 0)
        softerror("Unable to create tstr in delive");
    js_set_encode(tstr,JS_US_ASCII);
    if((lower = js_create(256,1)) == 0)
        softerror("Unable to create lower in delive");
    js_set_encode(lower,JS_US_ASCII);
    if((inprocess = js_create(256,1)) == 0)
        softerror("Unable to create inprocess in delive");
    js_set_encode(inprocess,JS_US_ASCII);

    /* Get the needed kvars */
    js_qstr2js(tstr,"kiwi_forward");
    read_kvar(tstr,do_forward);
    js_qstr2js(tstr,"kiwi_append");
    read_kvar(tstr,do_append);
    js_qstr2js(tstr,"kiwi_my_address");
    read_kvar(tstr,forwarder);
    js_qstr2js(tstr,"kiwi_my_mailbox");
    read_kvar(tstr,appender);
    js_qstr2js(tstr,"kiwi_inbound_process");
    read_kvar(tstr,inprocess);

    /* Determine if we are to forward the mail on */    
    js_copy(do_forward,lower);
    js_tolower(lower);
    js_qstr2js(tstr,"true");
    /* If so, forward the mail */
    if(js_fgrep(tstr,lower) == 0) {
        /* Sanity check */
	js_qstr2js(tstr,"@");
	if(js_fgrep(tstr,forwarder) <= 0)
	    softerror("kiwi_my_address address needs an @ in it");
        /* Mail forwarding */
        /* If kiwi_inbound_process is not defined, use kiwi_sendmail_path as
           the process to forward mail with */
        if(js_length(inprocess) == 0)
            forward(header,forwarder);
        /* Otherwise, use kiwi_inbound_process to deliver the message */
        else
            piper(header,forwarder,inprocess);
	/* We should never get here, but if we do... */
	exit(0);
	}

    /* Determine if we are to append mail */
    js_copy(do_append,lower);
    js_tolower(lower);
    js_qstr2js(tstr,"true");
    /* If so append the message */
    if(js_fgrep(tstr,lower) == 0) {
        /* Sanity Check */
	js_qstr2js(tstr,"/");
	if(js_fgrep(tstr,appender) != 0)
	    softerror("kiwi_my_mailbox filename needs a leading / in it");
        /* Mailbox appending */
	append(header,appender);
	/* We should never get here, but if we do... */
	exit(0);
	}
    else {
        softerror("Neither kiwi_forward nor kiwi_append is set");
        }

    /* We should never get here, but just in case... */
    exit(0);
    }

/* Forward: Send the email message on to the address in question
   input: header: Header of mail to send
          Email: Address to send mail to
   output: void function
*/

void forward(js_string *header, js_string *email) {
 
    js_string *sendmail, *tstr;
    /* Initialize strings */
    if((sendmail = js_create(256,1)) == 0)
        softerror("Unable to create sendmail string in forward");
    js_set_encode(sendmail,JS_US_ASCII);
    if((tstr = js_create(256,1)) == 0)
        softerror("Unable to create tstr string in forward");
    js_set_encode(tstr,JS_US_ASCII);

    /* Grab sendmail from the kvars */
    js_qstr2js(tstr,"kiwi_sendmail_path");
    read_kvar(tstr,sendmail);

    /* Run piper */
    piper(header,email,sendmail);

    /* We should never get here */
    harderror("How did we get here in forward");
    }

/* Piper: Send the header and the standard input to the process with the 
         single argument email
   input: A header to the data we send to the pipe, the first argument
          to the program (email), the program we pipe to (sendmail)
   output: void function
*/

void piper(js_string *header, js_string *email, js_string *sendmail) {
    js_string *line, *tstr, *qmailp;
    /* File descriptors for piping */
    int stream[2];
    /* Booleans */
    int is_qmail = 0;
    /* null-t strings */
    char sendmail_cmd[256];
    char email_addr[256];
    
    /* Initialize the strings */
    if((line = js_create(256,1)) == 0)
        softerror("Unable to create line in forward");
    js_set_encode(line,JS_US_ASCII);
    if((tstr = js_create(256,1)) == 0)
        softerror("Unable to create tstr in forward");
    js_set_encode(tstr,JS_US_ASCII);
    if((qmailp = js_create(256,1)) == 0)
        softerror("Unable to create qmailp in forward");
    js_set_encode(qmailp,JS_US_ASCII);

    /* Determine if we are running qmail */
    js_qstr2js(tstr,"kiwi_qmail");
    read_kvar(tstr,qmailp);
    js_tolower(qmailp);
    js_qstr2js(tstr,"true");
    if(js_fgrep(tstr,qmailp) == 0) 
        is_qmail = 1;

    /* Lots of cut and paste from the wrapper code above */

    /* Set up things to pipe stdout to the sendmail child process */
    if(pipe(stream) != 0)
        softerror("Unable to initialize pipe in forward");
    
    if(fork()) {
        /* Parent */
	close(stream[1]); /* close the input stream */
        dup2(stream[0],0); /* duplicate the standard input */
	if(js_js2str(sendmail,sendmail_cmd,200) == JS_ERROR)
	    harderror("Can not convert sendmail command in forward");
	if(js_js2str(email,email_addr,200) == JS_ERROR)
	    harderror("Can not convert email address in forward");
        execl(sendmail_cmd,sendmail_cmd,email_addr,(char *)0); 
	/* If we get here, there is a problem */
	harderror("Can not start sendmail in forward");
	}
    else {
        /* Child */

        close(stream[0]); /* Close the input stream */ 
        dup2(stream[1],1); /* duplicate the standard output to the pipe.
                              In other words, the standard output now goes
                              in to the pipe (sendmail) instead of to the
                              tty of the process */

        /* Display the DTLINE env variable, if we are running Qmail */
        if(is_qmail && getenv("DTLINE"))
	    printf("X-%s",getenv("DTLINE"));

        /* Display the header */
	js_show_stdout(header);

        /* Display the body of the message */
	printf("%s","\n"); /* May not be needed */

        while(!feof(stdin)) {
	    if(js_getline_stdin(line) == JS_ERROR)
	        harderror("Problem reading line from stdin");

            if(feof(stdin))
	        break;

            js_show_stdout(line);
	    }
        exit(0); 
	}

    /* We should never get here, but just in case... */
    exit(0);
    }

/* Append: Append a given email to a file, performing flock() on the file
   input: header: Header of mail to send
          file: file to append to
   output: void function
*/

void append(js_string *header, js_string *file) {
    js_string *tstr;
    js_file *desc;

    /* This is only a placeholder */
    /* softerror("Append is not supported yet"); */

    /* Set up the variables */
    desc = js_alloc(1,sizeof(js_file));
    if(desc == 0)
        softerror("Unable to allocate memory for desc in append");
    if((tstr = js_create(256,1)) == 0)
        softerror("Unable to create desc in append");
    js_set_encode(tstr,JS_US_ASCII);

    /* Append the mail */
    if(js_open_append(file,desc) == JS_ERROR)
        softerror("Unable to open mailbox for appending.");
    js_lock(desc);
    js_write(desc,header);
    while(!feof(stdin)) {
        if(js_getline_stdin(tstr) == JS_ERROR) 
	    harderror("Problem getting line from mail body.");
        if(!feof(stdin))
            js_write(desc,tstr);
        }
    js_unlock(desc);
    
    exit(0);
    }

