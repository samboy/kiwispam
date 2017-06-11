OBJECTS = JsStr.o JsStrOS.o JsStrCP.o KiwiJS.o
FLAGS = -O
#FLAGS = -g

all: tests

clean:
	rm -f core $(OBJECTS) tests

KiwiJS.o: JsStr.h KiwiJS.c
	cc -c $(FLAGS) -o KiwiJS.o KiwiJS.c 

JsStr.o: JsStr.h JsStr.c
	cc -c $(FLAGS) -o JsStr.o JsStr.c 

JsStrOS.o: JsStr.h JsStrOS.c
	cc -c $(FLAGS) -o JsStrOS.o JsStrOS.c 

JsStrCP.o: JsStr.h JsStrCP.c
	cc -c $(FLAGS) -o JsStrCP.o JsStrCP.c 

tests: tests.c $(OBJECTS)
	cc $(FLAGS) -o tests tests.c $(OBJECTS)

#onetime: $(OBJECTS) kiwi.h config.h onetime.c do_scramble.o cryptdate.o
#	cc $(FLAGS) -o onetime onetime.c do_scramble.o cryptdate.o

