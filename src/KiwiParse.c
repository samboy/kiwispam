/* Put in the public domain 2000 by Sam Trenholme */
/* Kiwi Parse: A series of programs that parse the .kiwirc file and set the
   relevent global variables */

#include "JsStr/JsStr.h"
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>

/* Alphanumberic digits */
#define AN "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-_"

/* Keywords that are significant to Kiwi's rc file */

#define KEYCOUNT 17

char *keywords[KEYCOUNT] = { 
	"kiwi_qmail", 
	"kiwi_sendmail_path", 
	"kiwi_days_short",
	"kiwi_days_mid",
	"kiwi_days_long",
	"kiwi_forward",
	"kiwi_append",
	"kiwi_my_mailbox",
	"kiwi_my_address",
	"kiwi_maillog",
	"kiwi_key",
	"kiwi_password",
	"kiwi_mid_process",
        "kiwi_bouncemail",
        "kiwi_inbound_process",
	"kiwi_utf8_output",
        "kiwi_return_path" };

js_string *kvar[KEYCOUNT];

/* kiwi_goodjs: Determine if a given js_string object is a valid string
                for use in the Kiwi internals
   input: Pointer to js_string object to test
   ouput: JS_ERROR if bad, JS_SUCCESS if good */
int kiwi_goodjs(js_string *test) {
    if(js_has_sanity(test) == JS_ERROR)
        return JS_ERROR;
    if(js_get_encode(test) != JS_US_ASCII)
        return JS_ERROR;
    return JS_SUCCESS;
    }

/* keyword2num: Convert a keyword (like "kiwi_maillog") to a number
                (10, in this case)
   input: A js_string object with the keyword
   output: The number of the keyword (starting at 0), JS_ERROR on error,
           -2 on no match 
   global vars used: keywords[]
*/
int keyword2num(js_string *keyword) {
    int counter = 0;
    js_string *name;

    if(kiwi_goodjs(keyword) == JS_ERROR)
        return JS_ERROR;

    if((name = js_create(256,1)) == 0)
        return JS_ERROR;

    js_set_encode(name,JS_US_ASCII);

    while(counter<KEYCOUNT) {
        if(js_qstr2js(name,keywords[counter]) == JS_ERROR) {
	    js_destroy(name);
	    return JS_ERROR;
            }
	if(js_issame(keyword,name)) {
            js_destroy(name);
	    return counter;
            }
        counter++;
        }
   
    js_destroy(name); 
    return -2;
    }

/* num2keyword: convert a number in to a keyword
   input: A number to make the keyword, the place to store the keyword
   output: JS_ERROR if it is out of range or any other error, JS_SUCCESS
           otherwise 
   global vars used: keywords[]
*/
int num2keyword(int num, js_string *keyword) {
    if(kiwi_goodjs(keyword) == JS_ERROR)
        return JS_ERROR;

    if(num < 0 || num >=KEYCOUNT)
        return JS_ERROR;

    return js_qstr2js(keyword,keywords[num]);
    }

/* init_kvars: Initialize the Kiwi variables that the Kiwi program will
               use in its operation 
   input: none
   output: JS_SUCCESS or JS_ERROR, depending on error/success
   global vars used: js_string kvar[KEYCOUNT] */
int init_kvars() {
    int counter;
    for(counter = 0;counter < KEYCOUNT;counter++) {
    	if((kvar[counter] = js_create(256,1)) == 0)
	    return JS_ERROR;
        js_set_encode(kvar[counter],JS_US_ASCII);
	}
    }

/* read_kvar: Put the value of the Kiwi variable with the name name in
              the value value.
   input: name, value
   output: JS_SUCCESS or JS_ERROR, depending on error/success
   global vars used: kvar[], keywords[]
*/
int read_kvar(js_string *name, js_string *value) {

    int num;
    num = keyword2num(name);

    /* Sanity checks */
    if(kiwi_goodjs(name) == JS_ERROR)
        return JS_ERROR;
    if(kiwi_goodjs(value) == JS_ERROR)
        return JS_ERROR;
    if(num == JS_ERROR)
        return JS_ERROR;

    /* Return not found if not found */
    if(num == -2)
        return num;

    /* Copy over the value */
    if((js_copy(kvar[num],value)) == JS_ERROR)
        return JS_ERROR;

    return JS_SUCCESS;

    }

/* write_kvar: Set the value of the Kiwi variable with the name name with
               the value value.
   input: name, value
   output: JS_SUCCESS or JS_ERROR, depending on error/success
   global vars used: kvar[], keywords[]
*/
int write_kvar(js_string *name, js_string *value) {

    int num;
    num = keyword2num(name);
    
    /* Sanity checks */
    if(kiwi_goodjs(name) == JS_ERROR)
        return JS_ERROR;
    if(kiwi_goodjs(value) == JS_ERROR)
        return JS_ERROR;
    if(num == JS_ERROR)
        return JS_ERROR;

    /* Return not found if not found */
    if(num == -2)
        return num;

    /* Copy over the value */
    if((js_copy(value,kvar[num])) == JS_ERROR)
        return JS_ERROR;

    return JS_SUCCESS;

    }

/* Parseline: Given a line of the Kiwirc file, parse that line
   input: A js_String object pointing to the contents of the line,
          A js_string object that will contain the name of the variable
          on the line ("ERROR" on syntax error),
          A js_string object that will contain the variable's value
          (description of problem on syntax error)
   output: JS_ERROR on fatal error, JS_SUCCESS otherwise */
int parseline(js_string *line, js_string *var, js_string *value) {

    static js_string *quotes = 0, *alphanumeric = 0, 
           *equals = 0, *hashq = 0, *bslashq = 0, *allq = 0;
    int quote1, quote2; /* Location of quotes in js_string line */
    int varstart,varend, valstart, valend; /* Pointers to beginning and end
                                              of the variable name and the
                                              value for the variable */
    int equalp, hashp; /* Location on equals sign and of hash */
    int tempp; /* temporary pointer */
    char quote = '"';
    char equal = '=';
    char hash = '#';
    char bslash = '\\';

    /* Sanity checks */
    if(kiwi_goodjs(line) == JS_ERROR)
        return JS_ERROR;
    if(kiwi_goodjs(var) == JS_ERROR)
        return JS_ERROR;
    if(kiwi_goodjs(value) == JS_ERROR)
        return JS_ERROR;
    /* Alocate strings */
    if(quotes == 0) 
    	if((quotes = js_create(256,1)) == 0)
        	return JS_ERROR;
    if(alphanumeric == 0)
    	if((alphanumeric = js_create(256,1)) == 0)
        	return JS_ERROR;
    if(equals == 0)
    	if((equals = js_create(256,1)) == 0)
        	return JS_ERROR;
    if(hashq == 0)
    	if((hashq = js_create(256,1)) == 0)
        	return JS_ERROR;
    if(bslashq == 0)
    	if((bslashq = js_create(256,1)) == 0)
        	return JS_ERROR;
    if(allq == 0)
    	if((allq = js_create(256,1)) == 0)
        	return JS_ERROR;

    /* Initialize the various sets we look for */
    js_str2js(equals,&equal,1,1); 
    js_str2js(hashq,&hash,1,1); 
    js_str2js(bslashq,&bslash,1,1); 
    js_qstr2js(alphanumeric,AN);
    /* AllQ is the union of all parsable characters */
    js_set_encode(allq,JS_US_ASCII);
    js_set_encode(quotes,JS_US_ASCII);
    js_space_chars(allq);
    /* Temporary usage of quotes string to store set of newlines to append */
    js_newline_chars(quotes);
    js_append(quotes,allq);
    /* Give quotes it correct value now */
    js_str2js(quotes,&quote,1,1); 
    js_append(quotes,allq);
    js_append(equals,allq);
    js_append(hashq,allq);
    js_append(bslashq,allq);
    js_append(alphanumeric,allq);

    /* Initialize the return values to nulls */
    js_qstr2js(var,"");
    js_qstr2js(value,"");

    /* By defulat, no quotes on the line */ 
    quote1 = quote2 = -2;    

    /* Locate the quotes */
    if((quote1 = js_match(quotes,line)) == JS_ERROR)
        return JS_ERROR;
    if(quote1 != -2)
        quote2 = js_match_offset(quotes,line,quote1 + 1);
   
    /* Locate the hash */
    if((hashp = js_match(hashq,line)) == JS_ERROR)
        return JS_ERROR;
    /* Look for it beyond last quote if found in quotes */
    if(hashp > quote1 && hashp < quote2)
        hashp = js_match_offset(hashq,line,quote2);
    /* Take it beyond the end of the line if not found */
    if(hashp == -2)
        hashp = js_length(line) + 1;
    /* If the quotes are after the hash, remove them */
    if(quote1 > hashp)
        quote1 = quote2 = -2;

    /* Error out on any unsupported characters not between quotes nor after
       the hash */
    if((tempp = js_notmatch(allq,line)) == JS_ERROR)
        return JS_ERROR;
    if((tempp < quote1 || tempp > quote2) && tempp < hashp && tempp != -2) {
        js_qstr2js(var,"ERROR");
        js_qstr2js(value,"Unknown character in line");
        return JS_SUCCESS;
        } 

    /* Error out if there is previsely one, or  more than two quotes on a line 
     */
    if(quote1 != -2) {
        if(quote2 == -2) {
            js_qstr2js(var,"ERROR");
	    js_qstr2js(value,"Quoted expression needs to be unquoted");
            return JS_SUCCESS;
            }
        if((tempp = js_match_offset(quotes,line,quote2 + 1)) == JS_ERROR)
            return JS_ERROR;
        if(tempp != -2 && tempp < hashp) {
            js_qstr2js(var,"ERROR");
	    js_qstr2js(value,"Maximum allowed quotes on a line is two");
            return JS_SUCCESS;
            }
        }

    /* Locate the equals */
    if((equalp = js_match(equals,line)) == JS_ERROR)
        return JS_ERROR;

    /* Find the beginning and end of the first word */
    varstart = varend = -2;
    if((varstart = js_match(alphanumeric,line)) == JS_ERROR)
        return JS_ERROR; 
    if(varstart != -2 && varstart < hashp) {
        varend = varstart;
        while(js_match_offset(alphanumeric,line,varend + 1) == varend + 1)
	    varend++;
        }
    else if(varstart >= hashp)
        varstart = -2;
    /* Doesn't count if you are in quotes -- this is a syntax error */
    if(varstart > quote1 && varend < quote2) {
        js_qstr2js(var,"ERROR");
        js_qstr2js(value,"Variable name in quotes");
        return JS_SUCCESS;
        }

    /* find the beginning and end of the second word */
    valstart = valend = -2;
    if(quote1 != -2 && quote2 != -2) {
        valstart = quote1 + 1;
	valend = quote2 - 1;
        /* We need to make sure there are no bare words between varend and 
           the first quote, allowing 'foo bar = "baz"' to return an error */
        if(js_match_offset(alphanumeric,line,varend + 1) < quote1) {
            js_qstr2js(var,"ERROR");
            js_qstr2js(value,"Second bare word found before quotes");
            return JS_SUCCESS;
            } 
	}
    else {
        if((valstart = js_match_offset(alphanumeric,line,varend + 1)) == 
	   JS_ERROR)
	    return JS_ERROR;
        if(valstart != -2 && valstart < hashp) {
	    valend = valstart;
	    while(js_match_offset(alphanumeric,line,valend + 1) == valend + 1)
	        valend++;
            }
        else if(valstart >= hashp)
	    valstart = valend = -2;
        }

    /* return error if there is backslash in line */
    if(js_match(bslashq,line) != -2) {
        js_qstr2js(var,"ERROR");
        js_qstr2js(value,"Backslash not supported yet");
        return JS_SUCCESS;
        }
        
    /* Now that va[rl]start and va[rl]end have values, do the substrs */

    if(varstart >=0 && varend >= varstart) {
        /* Syntax error if no equals sign */
        if(equalp == -2) {
	    js_qstr2js(var,"ERROR");
	    js_qstr2js(value,"Variable name needs an = sign");
	    return JS_SUCCESS;
	    }
        /* Syntax error if before equals sign */
        if(varstart > equalp) {
            js_qstr2js(var,"ERROR");
            js_qstr2js(value,"Equals sign before variable name");
            return JS_SUCCESS;
            }
        if((js_substr(line,var,varstart,varend - varstart + 1)) == JS_ERROR)
            return JS_ERROR;
        }
    
    if(valstart >=0 && valend >= valstart) {
        /* Syntax error if first equals on line is in quotes */
        if(equalp > quote1 && equalp < quote2) {
            js_qstr2js(var,"ERROR");
            js_qstr2js(value,"Quoted string not preceeded by equals sign");
            return JS_SUCCESS;
            } 
        /* Syntax error if after equals sign */
        if(valstart < equalp) {
            js_qstr2js(var,"ERROR");
            js_qstr2js(value,"Equals sign after variable value");
            return JS_SUCCESS;
            }
        if((js_substr(line,value,valstart,valend - valstart + 1)) == JS_ERROR)
            return JS_ERROR;
        }

    /* Syntax error if any weird stuff exists after end of variable name */
    js_space_chars(allq);
    js_append(quotes,allq);
    js_newline_chars(quotes);
    js_append(quotes,allq);
    tempp = js_notmatch_offset(allq,line,valend + 1);
    if(tempp < hashp && tempp != -2) {
        js_qstr2js(var,"ERROR");
        js_qstr2js(value,"Unexpected character near end of line");
        return JS_SUCCESS;
        }

    return JS_SUCCESS;

    }

/* find_kiwirc: Find the kiwirc file we are supposed to read
           Input: js_string to place kiwirc file in
           Output: JS_ERROR on error, JS_SUCCESS on success
*/
int find_kiwirc(js_string *out) {
  
    struct passwd *info;
    js_string *temp;
    int found = 0;
    uid_t uid;

    /* Sanity checks */
    if(kiwi_goodjs(out) == JS_ERROR)
        return JS_ERROR;

    /* First, we look for the environmental variable KIWIRC */
    if(getenv("KIWIRC") != NULL) {
        if((js_qstr2js(out,getenv("KIWIRC"))) == JS_ERROR)
	    return JS_ERROR;
        return JS_SUCCESS;
        }

    /* Ugh, OK do we point to the home directory with the env variable
       HOME? */
    if(getenv("HOME") != NULL) {
        if((js_qstr2js(out,getenv("HOME"))) == JS_ERROR)
	    return JS_ERROR;
        found = 1;
	}
    else { /* We're getting desperate.  Lets try to find their home 
              directory in /etc/passwd */
        info = getpwent();
	uid = geteuid();
	while(info != NULL) {
	    if(info->pw_uid == uid) { /* Bingo */
	        if((js_qstr2js(out,info->pw_dir)) == JS_ERROR)
		    return JS_ERROR;
                found = 1;
                break;
		}
            info = getpwent();
            }
        }

    if(found == 0) /* If we don't have a home directory */ 
        return JS_ERROR; /* Then we don't exist.  That is illocical, captain */

    /* Append "/.kiwirc" to the home directory */
    temp = js_create(256,1);
    js_qstr2js(temp,"/.kiwirc");
    js_append(temp,out);

    return JS_SUCCESS;

    }

/* read_kwirc: Read the .kiwirc file, and set the appropriate kiwi symbols
   input: Name of .kiwirc file, place to put error string (if needed),
          place to put error number (0 if no error)
   output: JS_ERROR on error, JS_SUCCESS on success
   global vars: kvar
*/

int read_kiwirc(js_string *fileloc,js_string *errorstr,int *errorret) {
    static js_string *line = 0;
    static js_string *var = 0; 
    static js_string *value = 0;
    static js_string *tstr = 0; /* Temporary string */

    int error = 0; /* Line error is found on */
    int linenum = 1;
    int tnum; /* temporary number */

    static js_file *file = 0;

    *errorret = -1; /* Fatal error */

    /* Allocate memory for the variables */
    if(line == 0)
        if((line = js_create(256,1)) == 0) {
	    js_qstr2js(errorstr,"Fatal error creating js_string");
	    return JS_ERROR;
	    }
    if(var == 0)
        if((var = js_create(256,1)) == 0) {
	    js_qstr2js(errorstr,"Fatal error creating js_string");
	    return JS_ERROR;
	    }
    if(value == 0)
        if((value = js_create(256,1)) == 0) {
	    js_qstr2js(errorstr,"Fatal error creating js_string");
	    return JS_ERROR;
	    }
    if(tstr == 0)
        if((tstr = js_create(256,1)) == 0) {
	    js_qstr2js(errorstr,"Fatal error creating js_string");
	    return JS_ERROR;
	    }
    if(file == 0)
        if((file = js_alloc(1,sizeof(js_file))) == 0) {
	    js_qstr2js(errorstr,"Fatal error creating file");
	    return JS_ERROR;
	    }

    /* Initialize values */
    js_qstr2js(errorstr,"");
    js_set_encode(line,JS_US_ASCII);
    js_set_encode(var,JS_US_ASCII);
    js_set_encode(value,JS_US_ASCII);
    js_set_encode(tstr,JS_US_ASCII);
    if(js_has_sanity(kvar[0]) == JS_ERROR)
        init_kvars();

    /* Start reading and processing lines from the file */
    js_open_read(fileloc,file);
    while(!js_buf_eof(file)) {
        if((js_buf_getline(file,line)) == JS_ERROR) {
	    js_qstr2js(errorstr,"Fatal error calling js_buf_getline");
	    return JS_ERROR;
	    }
	if((parseline(line,var,value)) == JS_ERROR) {
	    js_qstr2js(errorstr,"Fatal error calling parseline");
	    return JS_ERROR;
	    }
        tnum = keyword2num(var);
	if(tnum == JS_ERROR) {
	    js_qstr2js(errorstr,"Fatal error calling keyword2num");
	    return JS_ERROR;
	    }
        if(tnum == -2) { /* If the symbol was not found */
	    js_qstr2js(tstr,"ERROR");
	    if(js_issame(tstr,var)) {
	        if(!error) {
	            error = linenum;
                    *errorret = error;
	            js_copy(value,errorstr);
		    }
	        } 
            else {
                if(!error && js_length(var) > 0) {
	            error = linenum;
                    *errorret = error;
	            js_qstr2js(errorstr,"Unknown variable name found");
		    }
		}
	    }
        else if(!error) 
	    write_kvar(var,value); 

	linenum++;
        } 

    if(!error)
        *errorret = 0;

    }

