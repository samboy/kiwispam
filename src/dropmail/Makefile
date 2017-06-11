JOBJS=../JsStr/JsStr.o ../JsStr/JsStrOS.o ../JsStr/JsStrCP.o
EXECS=dropmail 

FLAGS = -O
#FLAGS = -g

all: ../JsStr/tests $(EXECS)

clean:
	rm -f $(EXECS) ; cd ../JsStr ; make clean

../JsStr/tests:
	cd ../JsStr ; make

dropmail: dropmail.c dropmail.h $(JOBJS) 
	cc $(FLAGS) -o dropmail dropmail.c $(JOBJS)

#install: clicrypt
#	./install.sh

