#!/bin/sh        

# Reduce the size of the binary
strip clicrypt 

# Copy it over to /usr/local/bin
cp clicrypt /usr/local/bin 

# Handle the case of a system running sendmail
# we assume the directory /var/spool/mail is owned by the group "mail"
if [ -e /etc/smrsh ] ; then
	cp clicrypt /etc/smrsh/infilter
	chmod 755 /etc/smrsh/infilter
fi

# Make the sym links in /usr/local/bin
cd /usr/local/bin 
rm infilter wrapper decode 2> /dev/null
ln -s clicrypt infilter
ln -s clicrypt wrapper 
ln -s clicrypt decode
chmod 755 clicrypt
