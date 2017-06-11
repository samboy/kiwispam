OBJECTS=KiwiParse.o
JOBJS=JsStr/JsStr.o JsStr/JsStrOS.o JsStr/JsStrCP.o
KJOBJS=JsStr/KiwiJS.o
EXECS=clicrypt decode wrapper infilter

FLAGS = -O
#FLAGS = -g

all: JsStr/tests KiwiParse.o parsetest $(EXECS)

clean:
	rm -f $(OBJECTS) core parsetest $(EXECS) ; cd JsStr ; make clean

JsStr/tests:
	cd JsStr ; make

KiwiParse.o: KiwiParse.c JsStr/JsStr.h
	cc -c $(FLAGS) -o KiwiParse.o KiwiParse.c 

parsetest: parsetest.c KiwiParse.o $(OBJECTS) $(JOBJS) $(KJOBJS)
	cc $(FLAGS) -o parsetest parsetest.c $(OBJECTS) $(JOBJS) $(KJOBJS)

clicrypt: KiwiMain.c $(OBJECTS) $(JOBJS) $(KJOBJS)
	cc $(FLAGS) -o clicrypt KiwiMain.c $(OBJECTS) $(JOBJS) $(KJOBJS)

decode: clicrypt
	ln -s clicrypt decode

wrapper: clicrypt
	ln -s clicrypt wrapper

infilter: clicrypt
	ln -s clicrypt infilter

install: clicrypt
	./install.sh

