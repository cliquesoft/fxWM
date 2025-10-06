#!/bin/sh

if [ "$(uname -m)" == 'x86_64' ] && [ "$1" != '--help' ]; then
	# set compile flags: 64-bit
	echo "Setting compile flags to 64bit CPU..."

	export CFLAGS="-mtune=generic -Os -pipe"
	export CXXFLAGS="-mtune=generic -Os -pipe -fno-exceptions -fno-rtti"
	export LDFLAGS="-Wl,-O1"
elif [ "$1" != '--help' ]; then
	# set compile flags: 32-bit
	echo "Setting compile flags to 32bit CPU..."

	export CFLAGS="-march=i486 -mtune=i686 -Os -pipe"
	export CXXFLAGS="-march=i486 -mtune=i686 -Os -pipe"
	export LDFLAGS="-Wl,-O1"
fi
export CXXFLAGS="$CXXFLAGS $(fltk-config --cxxflags)"
export CXXFLAGS="$CXXFLAGS -Wall -ffunction-sections -fdata-sections -Wno-strict-aliasing"
export LDFLAGS="$LDFLAGS $(fltk-config --ldstaticflags --use-images)"
#export LDFLAGS="$LDFLAGS -Wl,-gc-sections"

# export directory paths
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

SOFT='fxwm'
PREFIX=''
INSTALL=0




# check that the correct parameters have been sent to the script
if [ "$1" == '--help' ]; then
	echo
	echo "${0##*/} [PREFIX=/directory] [install]"
	echo
	echo "   PREFIX	the directory to use during installation/staging"
	echo "   install	indicates the software should be installed afterwards"
	echo
	exit 0
elif ( echo "$1" | grep -q ^'PREFIX=' ); then
	PREFIX="${1#*=}"
elif [ "$1" == 'install' ] || [ "$2" == 'install' ]; then
	INSTALL=1
fi

# check that the account has appropriate permissions to install the software
if [ $INSTALL -gt 0 ] && [ $EUID -gt 0 ]; then		# checks that this ACTION is being run as an elevated account (e.g. as root or using sudo) so we don't have any issues getting the directories all setup
	echo -e "\nThis install script was executed without sufficient priviledges. Try using a different account or 'sudo', exiting.\n"
	exit 1
fi




# check that the required packages are installed to compile the software
if [ "$(which g++)" == '' ]; then
	echo "ERROR: you are missing the 'g++' binary."
	exit 1
fi
if [ "$(which mksquashfs)" == '' ]; then
	echo "ERROR: you are missing the 'mksquashfs' binary."
	exit 1
fi


# define the required packages for this software to run
DEP=		#"fltk-1.3"




# compile the software
echo -e "\nCompiling the window manager..."
g++ -std=c++11 -o $SOFT *.C *.cpp $CXXFLAGS $LDFLAGS || {
	echo "ERROR: something went wrong during the compile process!"
	exit 0
}

# strip the debug symbols from the just-compiled software
find . | xargs file | grep "executable" | grep ELF | grep "not stripped" | cut -f 1 -d : | xargs strip --strip-unneeded 2> /dev/null || find . | xargs file | grep "shared object" | grep ELF | grep "not stripped" | cut -f 1 -d : | xargs strip -g 2> /dev/null

# install the software
echo -e "The software has compiled successfully!"

if [ $INSTALL -gt 0 ]; then
	echo -e "Installing the window manager..."
	[ ! -d "${PREFIX}/usr/bin" ] && mkdir -p "${PREFIX}/usr/bin"
	cp $SOFT "${PREFIX}/usr/bin"

	[ ! -d "${PREFIX}/usr/share/man/man1" ] && mkdir -p "${PREFIX}/usr/share/man/man1"
	gzip < $SOFT.1 > "${PREFIX}/usr/share/man/man1/$SOFT.1.gz"

	[ ! -d "${PREFIX}/etc/$SOFT" ] && mkdir -p "${PREFIX}/etc/$SOFT"
	cp chrome.css "${PREFIX}/etc/$SOFT"
	cp $SOFT "${PREFIX}/usr/bin"
fi

# last-call file structure manipulation
echo -e "You are now ready to use FXwm!"
[ "$PREFIX" == '' ] && exit 0				# if no PREFIX was given, then we aren't packaging the software...

echo -en "\nWould you like to package it? [Y/N] (N): "
read
[ "$(echo ${REPLY} | tr '[a-z]' '[A-Z]')" != 'Y' ] && exit 0




# create the wrapper script
cat >/tmp/$SOFT <<EOT
#!/bin/sh

# create necessary directories
[ ! -e "\${HOME}/.etc/$SOFT" ] && mkdir -p "\${HOME}/.etc/$SOFT"

# create default values
[ ! -e "\${HOME}/.etc/$SOFT/supervisor" ] && [ -e "/etc/$SOFT/supervisor" ] && ln -sf /etc/$SOFT/supervisor "\${HOME}/.etc/$SOFT/supervisor"
[ ! -e "\${HOME}/.etc/$SOFT/supervisor.conf" ] && [ -e "/etc/$SOFT/supervisor.conf" ] && ln -sf /etc/$SOFT/supervisor.conf "\${HOME}/.etc/$SOFT/supervisor.conf"
[ ! -e "\${HOME}/.etc/$SOFT/windows.conf" ] && [ -e "/etc/$SOFT/windows.conf" ] && ln -sf /etc/$SOFT/windows.conf "\${HOME}/.etc/$SOFT/windows.conf"

# create personalized/failsafe values
if [ "\$USER" != 'root' ] && [ -e /usr/local/var/www/web.de ]; then							# XiniX
	[ ! -e "\${HOME}/.etc/$SOFT/supervisor" ] && echo "$(which web.ui) -t web.de http://127.0.0.1:8181 >>/var/log/web.de.log 2>&1" >"\${HOME}/.etc/$SOFT/supervisor"
	[ ! -e "\${HOME}/.etc/$SOFT/supervisor.conf" ] && ln -sf "/usr/local/var/www/web.de/themes/default/config/supervisor.conf" "\${HOME}/.etc/$SOFT/supervisor.conf"
	[ ! -e "\${HOME}/.etc/$SOFT/windows.conf" ] && ln -sf "/usr/local/var/www/web.de/themes/default/config/windows.conf" "\${HOME}/.etc/$SOFT/windows.conf"
elif [ "\$USER" != 'root' ] && [ ! -e /usr/local/var/www/web.de ] && [ "\$(which wbar)" != '' ]; then			# TC
	[ ! -e "\${HOME}/.etc/$SOFT/supervisor" ] && echo "$(which wbar) >>/var/log/wbar.log 2>&1" >"\${HOME}/.etc/$SOFT/supervisor"
	[ ! -e "\${HOME}/.etc/$SOFT/supervisor.conf" ] && touch "\${HOME}/.etc/$SOFT/supervisor.conf"
	[ ! -e "\${HOME}/.etc/$SOFT/windows.conf" ] && echo "-o 0:0:0:0" >"\${HOME}/.etc/$SOFT/windows.conf"
elif [ "\$USER" == 'root' ]; then											# these are the values for the authentication screen of web.de
	[ ! -e "\${HOME}/.etc/$SOFT/supervisor" ] && echo "$(which web.ui) -t web.de http://127.0.0.1:8181 >>/var/log/web.de.log 2>&1" >"\${HOME}/.etc/$SOFT/supervisor"
	[ ! -e "\${HOME}/.etc/$SOFT/supervisor.conf" ] && echo "-F -c" >"\${HOME}/.etc/$SOFT/supervisor.conf"
	[ ! -e "\${HOME}/.etc/$SOFT/windows.conf" ] && echo "-o 0:0:0:0" >"\${HOME}/.etc/$SOFT/windows.conf"		# NOTE: this segment will go away once fxwm is not started until a login has been made!!!
fi

# execute any executables present for the WM
IFS=\$'\n'
for FILE in \$(find -L \${HOME}/.etc/$SOFT -maxdepth 1 -type f -perm /u=x ! -perm /o=w); do
	setsid "\$FILE" >>/var/log/${SOFT}.log 2>&1
done

# start the WM and cleanup
echo '#!/bin/sh' >/tmp/wm
echo "exec .$SOFT -S \"\$(cat "\${HOME}/.etc/fxwm/supervisor")\" \$(cat "\${HOME}/.etc/fxwm/supervisor.conf") \$(cat "\${HOME}/.etc/fxwm/windows.conf") >>/var/log/${SOFT}.log 2>&1" >>/tmp/wm
chmod 755 /tmp/wm
/tmp/wm &
sleep 5
rm /tmp/wm
EOT
chmod 755 /tmp/$SOFT


# creating the package files for the proper Linux
echo -en "Package $SOFT for XiniX Linux? [Y/N] (N): "
read
if [ "$(echo ${REPLY} | tr '[a-z]' '[A-Z]')" == 'Y' ]; then
	echo "Creating the proper packages..."

	mkdir -p ${PREFIX}/usr/local/bin ${PREFIX}/usr/local/share/man/man1 ${PREFIX}/var/cache/Software/scripts
	cp /tmp/${SOFT} ${PREFIX}/usr/local/bin
	cp ${SOFT} ${PREFIX}/usr/local/bin/.${SOFT}			# NOTE: we have to have an alternative name for the binary to pass parameters to the WM
	cp ${SOFT}.1 ${PREFIX}/usr/local/share/man/man1
	echo -e "#!/bin/sh\necho $SOFT >/etc/software/SYSTEM/wm" >>${PREFIX}/var/cache/Software/scripts/${SOFT}.bin
	chmod 755 ${PREFIX}/var/cache/Software/scripts/${SOFT}.bin

	cd "$PREFIX" && mksquashfs . "/tmp/${SOFT}.i64.bin.soft" && cd /tmp
	md5sum "${SOFT}.i64.bin.soft" >"${SOFT}.i64.bin.hash"		# create the md5 checksum file
	echo -e $DEP > "${SOFT}.i64.bin.deps"				# create the dependency file
	cd "$PREFIX" && find . -not -type d > "../${SOFT}.i64.bin.list"	# create the manifest
fi

echo -en "\nPackage $SOFT for TinyCore Linux? [Y/N] (N): "
read
if [ "$(echo ${REPLY} | tr '[a-z]' '[A-Z]')" == 'Y' ]; then
	echo "Creating the proper packages..."

	mkdir -p ${PREFIX}/usr/local/bin ${PREFIX}/usr/local/share/man/man1 ${PREFIX}/usr/local/tce.installed
	cp /tmp/${SOFT} ${PREFIX}/usr/local/bin
	cp ${SOFT} ${PREFIX}/usr/local/bin/.${SOFT}
	cp ${SOFT}.1 ${PREFIX}/usr/local/share/man/man1
	echo -e "#!/bin/sh\necho $SOFT >/etc/sysconfig/desktop" >>${PREFIX}/usr/local/tce.installed/${SOFT}
	chmod 755 ${PREFIX}/usr/local/tce.installed/${SOFT}

	cd "$PREFIX" && mksquashfs . "/tmp/$SOFT.tcz" && cd /tmp
	md5sum "$SOFT.tcz" > "$SOFT.tcz.md5.txt"
	echo -e $DEP > "$SOFT.tcz.dep"
	cd "$PREFIX" && find . -not -type d >"../$SOFT.tcz.list"
fi

echo -en "\nPackage $SOFT for LSB Linux? [Y/N] (N): "
read
if [ "$(echo ${REPLY} | tr '[a-z]' '[A-Z]')" == 'Y' ]; then
	echo "Creating the proper packages..."

	mkdir -p ${PREFIX}/usr/bin ${PREFIX}/usr/share/man/man1
	cp /tmp/${SOFT} ${PREFIX}/usr/bin
	cp ${SOFT} ${PREFIX}/usr/bin/.${SOFT}
	cp ${SOFT}.1 ${PREFIX}/usr/share/man/man1

	echo "The package contents are setup with the only steps remaining:"
	echo "  - Add the default supervisor to: /etc/$SOFT/supervisor"
	echo "  - Add the default parameters to:"
	echo "	/etc/$SOFT/supervisor.conf"
	echo "	/etc/$SOFT/windows.conf"
	echo "  - Use your distro's package manager to create the package"
	echo "	located in: $PREFIX"
fi

