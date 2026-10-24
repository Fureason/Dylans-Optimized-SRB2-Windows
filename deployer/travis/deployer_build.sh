#!/bin/bash






if [[ "$__DPL_FTP_ACTIVE" == "1" ]] || [[ "$__DPL_DPUT_ACTIVE" == "1" ]]; then
	if [[ "$__DPL_DEBIAN_ACTIVE" == "1" ]]; then
		echo "Building Debian package(s)"

		sudo apt-get install devscripts debhelper fakeroot secure-delete expect;



		if [[ "$_DPL_PACKAGE_MAIN" == "1" ]]; then
			. ../debian_template.sh main;
			OLDPWD=$PWD;  
			cd ..;  

			if [[ "$_DPL_PACKAGE_SOURCE" == "1" ]]; then
				echo "Building main source Debian package";
				expect <(cat <<EOD
spawn debuild -S -us -uc;
expect "continue anyway? (y/n)"
send "y\r"
interact
EOD
);
			fi;

			if [[ "$_DPL_PACKAGE_BINARY" == "1" ]]; then
				echo "Building main binary Debian package";
				expect <(cat <<EOD
spawn debuild -us -uc;
expect "continue anyway? (y/n)"
send "y\r"
interact
EOD
);
			fi;

			cd $OLDPWD;
		fi;


		if [[ "$_DPL_PACKAGE_ASSET" == "1" ]]; then
			. ../debian_template.sh asset;
			OLDPWD=$PWD;  
			cd ../assets;





			if [[ "$_DPL_PACKAGE_SOURCE" == "1" ]]; then
				echo "Building asset source Debian package";
				expect <(cat <<EOD
spawn debuild -S -us -uc;
expect "continue anyway? (y/n)"
send "y\r"
interact
EOD
);
			fi;

			if [[ "$_DPL_PACKAGE_BINARY" == "1" ]]; then
				echo "Building asset binary Debian package";
				expect <(cat <<EOD
spawn debuild -us -uc;
expect "continue anyway? (y/n)"
send "y\r"
interact
EOD
);
			fi;

			cd $OLDPWD;
		fi;


		if [[ "$DPL_PGP_KEY_PRIVATE" != "" ]] && [[ "$DPL_PGP_KEY_PASSPHRASE" != "" ]]; then


			echo "$DPL_PGP_KEY_PRIVATE" | base64 --decode > key.asc;
			echo "$DPL_PGP_KEY_PASSPHRASE" > phrase.txt;
			gpg --import key.asc;

			if [[ "$_DPL_PACKAGE_MAIN" == "1" ]]; then
				echo "Signing main package(s)";

				PACKAGEFILENAME=${PACKAGE_NAME}_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};
				PACKAGEDBGFILENAME=${PACKAGE_NAME}-dbg_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};







				PACKAGEFILENAMES=(
					$PACKAGEFILENAME
					$PACKAGEDBGFILENAME






				);


				OLDPWD=$PWD;  
				cd ../..;  

				for n in ${PACKAGEFILENAMES}; do
					for f in ./$n*.changes; do
						debsign --no-re-sign -p"gpg --passphrase-file $OLDPWD/phrase.txt --batch" "$f";
					done;
				done;

				cd $OLDPWD;
			fi;

			if [[ "$_DPL_PACKAGE_ASSET" == "1" ]]; then
				echo "Signing asset package(s)";

				PACKAGEFILENAME=${PACKAGE_NAME}-data_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};




				PACKAGEFILENAMES=(
					$PACKAGEFILENAME



				)


				OLDPWD=$PWD;  
				cd ..;  

				for n in ${PACKAGEFILENAMES}; do
					for f in ./$n*.changes; do
						debsign --no-re-sign -p"gpg --passphrase-file $OLDPWD/phrase.txt --batch" "$f";
					done;
				done;

				cd $OLDPWD;
			fi;


			srm key.asc;
			srm phrase.txt;
		fi;
	fi;


	if [[ "$TRAVIS_OS_NAME" != "linux" ]]; then



		if [[ "$_DPL_BINARY" == "1" ]]; then
			echo "Building a Binary";
			make -k;
		fi;




		if [[ "$_DPL_PACKAGE_BINARY" == "1" ]]; then
			echo "Building a Package";








			if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
				sudo make -k package;
			else

				make -k package;
			fi;
		fi;
	fi;
fi;
