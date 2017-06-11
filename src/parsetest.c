/* Public domain 200 by Sam Trenholme */
/* This tests the parsing of the KiwiParse routines */

#include "JsStr/JsStr.h"
#include <stdio.h>

main() {
	js_string *line, *v1, *v2, *v3;
	js_file *f;
        int c;

	line = js_create(256,1);
	v1 = js_create(256,1);
	v2 = js_create(256,1);
	v3 = js_create(256,1);
	f = js_alloc(1,sizeof(js_file));

	js_qstr2js(v1,"example_kiwirc");
        js_set_encode(v1,JS_US_ASCII);
        js_set_encode(v2,JS_US_ASCII);
        js_set_encode(v3,JS_US_ASCII);
        read_kiwirc(v1,v2);
        for(c=0;c<14;c++) {
		num2keyword(c,v1);
		read_kvar(v1,v2);
		js_show_stdout(v1);
		printf(" ");
		js_show_stdout(v2);
		printf("\n");
		}	
	}
