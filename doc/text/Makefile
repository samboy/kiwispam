
all: Spec.txt Files.txt Intro.txt Test_vectors.txt Faq.txt Start.txt \
	Webmailto.txt Bouncemail.txt Kiwirc.txt Env.txt

clean:
	rm -f *.txt

Spec.txt: ../html/Spec.html
	../../tools/procspec ../html/Spec.html > Spec.txt

Files.txt: ../html/Files.html
	lynx -dump -nolist ../html/Files.html > Files.txt

Intro.txt: ../html/Intro.html
	lynx -dump -nolist ../html/Intro.html > Intro.txt

Faq.txt: ../html/Faq.html
	lynx -dump -nolist ../html/Faq.html > Faq.txt

Test_vectors.txt: ../html/Test_vectors.html
	lynx -dump -nolist ../html/Test_vectors.html > Test_vectors.txt

Start.txt: ../html/Start.html
	lynx -dump -nolist ../html/Start.html > Start.txt

Webmailto.txt: ../html/Webmailto.html
	lynx -dump -nolist ../html/Webmailto.html > Webmailto.txt

Bouncemail.txt: ../html/Bouncemail.html
	lynx -dump -nolist ../html/Bouncemail.html > Bouncemail.txt

Kiwirc.txt: ../html/Kiwirc.html
	lynx -dump -nolist ../html/Kiwirc.html > Kiwirc.txt

Env.txt: ../html/Env.html
	lynx -dump -nolist ../html/Env.html > Env.txt

wrapper.1.txt: ../man/wrapper.1
	nroff -man -Tascii ../man/wrapper.1 | col -b > wrapper.1.txt

cryptday.1.txt: ../man/cryptday.1
	 nroff -man -Tascii ../man/cryptday.1 | col -b > cryptday.1.txt

decode.1.txt: ../man/decode.1
	nroff -man -Tascii ../man/decode.1 | col -b > decode.1.txt

ftpplace.1.txt: ../man/ftpplace.1
	nroff -man -Tascii ../man/ftpplace.1 | col -b > ftpplace.1.txt

infilter.1.txt: ../man/infilter.1
	nroff -man -Tascii ../man/infilter.1 | col -b > infilter.1.txt

ipcrypt.1.txt: ../man/ipcrypt.1
	nroff -man -Tascii ../man/ipcrypt.1 | col -b > ipcrypt.1.txt

msgcrypt.1.txt: ../man/msgcrypt.1
	nroff -man -Tascii ../man/msgcrypt.1 | col -b > msgcrypt.1.txt
