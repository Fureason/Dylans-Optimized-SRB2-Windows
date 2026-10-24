#!/bin/bash

















if [[ "$__DPL_FTP_ACTIVE" == "1" ]]; then
	if [[ "$TRAVIS_JOB_NAME" != "" ]]; then
		JOBNAME=$TRAVIS_JOB_NAME;
	else
		if [[ "$_DPL_JOB_NAME" != "" ]]; then
			JOBNAME=$_DPL_JOB_NAME;
		else
			JOBNAME=$TRAVIS_OS_NAME;
		fi;
	fi;


	echo "Travis-CI Build $TRAVIS_OS_NAME - $TRAVIS_REPO_SLUG/$TRAVIS_BRANCH - $TRAVIS_JOB_NUMBER - $JOBNAME" > "commit.txt";
	echo "Job ID $TRAVIS_JOB_ID" >> "commit.txt";
	echo "" >> "commit.txt";
	echo "Commit $TRAVIS_COMMIT" >> "commit.txt";
	echo "$TRAVIS_COMMIT_MESSAGE" >> "commit.txt";
	echo "" >> "commit.txt";


	if [[ "$DPL_FTP_PORT" == "" ]]; then
		DPL_FTP_PORT=21;
	fi;
	if [[ "$DPL_FTP_PROTOCOL" == "" ]]; then
		DPL_FTP_PROTOCOL=ftp;
	fi;
	__DPL_FTP_LOCATION=$DPL_FTP_PROTOCOL://$DPL_FTP_HOSTNAME:$DPL_FTP_PORT/$DPL_FTP_PATH/$TRAVIS_REPO_SLUG/$TRAVIS_BRANCH/$TRAVIS_JOB_ID-$TRAVIS_JOB_NUMBER-$JOBNAME;


	echo "Uploading to FTP...";
	curl --ftp-create-dirs -T "commit.txt" -u $DPL_FTP_USER:$DPL_FTP_PASS "$__DPL_FTP_LOCATION/commit.txt";

	if [[ "$__DPL_DEBIAN_ACTIVE" == "1" ]]; then
		if [[ "$_DPL_PACKAGE_MAIN" == "1" ]]; then
			PACKAGEFILENAME=${PACKAGE_NAME}_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};
			PACKAGEDBGFILENAME=${PACKAGE_NAME}-dbg_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};







			PACKAGEFILENAMES=(
				$PACKAGEFILENAME
				$PACKAGEDBGFILENAME






			);


			OLDPWD=$PWD;  
			cd ../..;

			for n in ${PACKAGEFILENAMES}; do
				for f in ./$n*; do


					if [[ "$_DPL_PACKAGE_SOURCE" != "1" ]]; then
						if [[ "$f" == *"_source"* ]] || [[ "$f" == *".tar.xz"* ]]; then
							continue;
						fi;
					fi;
					curl --ftp-create-dirs -T "$f" -u $DPL_FTP_USER:$DPL_FTP_PASS  "$__DPL_FTP_LOCATION/package/main/$f";
				done;
			done;


			cd $OLDPWD;
		fi;

		if [[ "$_DPL_PACKAGE_ASSET" == "1" ]]; then
			PACKAGEFILENAME=${PACKAGE_NAME}-data_${PACKAGE_VERSION}${PACKAGE_SUBVERSION}${PACKAGE_REVISION};




			PACKAGEFILENAMES=(
				$PACKAGEFILENAME



			)


			OLDPWD=$PWD;  
			cd ..;

			for n in ${PACKAGEFILENAMES}; do
				for f in ./$n*; do


					if [[ "$_DPL_PACKAGE_SOURCE" != "1" ]]; then
						if [[ "$f" == *"_source"* ]] || [[ "$f" == *".tar.xz"* ]]; then
							continue;
						fi;
					fi;
					curl --ftp-create-dirs -T "$f" -u $DPL_FTP_USER:$DPL_FTP_PASS  "$__DPL_FTP_LOCATION/package/asset/$f";
				done;
			done;


			cd $OLDPWD;
		fi;
	else
		if [[ "$_DPL_BINARY" == "1" ]]; then
			find bin -type f -exec curl -u $DPL_FTP_USER:$DPL_FTP_PASS --ftp-create-dirs -T {} $__DPL_FTP_LOCATION/{} \;;
		fi;

		if [[ "$_DPL_PACKAGE_BINARY" == "1" ]]; then
			sudo rm -r package/_CPack_Packages
			find package -type f -exec curl -u $DPL_FTP_USER:$DPL_FTP_PASS --ftp-create-dirs -T {} $__DPL_FTP_LOCATION/{} \;;
		fi;
	fi;
fi
