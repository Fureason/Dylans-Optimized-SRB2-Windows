#!/bin/bash


























if [[ "$DPL_ENABLED" == "1" ]] && [[ "$TRAVIS_PULL_REQUEST" == "false" ]]; then



    if [[ $TRAVIS_BRANCH == *"deployer"* ]]; then
        __DPL_BASE_ELIGIBLE=1;
        __DPL_TERMINATE_EARLY_ELIGIBLE=1;
    fi;

    if [[ "$TRAVIS_TAG" != "" ]] && [[ "$DPL_TAG_ENABLED" == "1" ]]; then
        __DPL_BASE_ELIGIBLE=1;
        __DPL_TAG_ELIGIBLE=1;
        __DPL_TERMINATE_EARLY_ELIGIBLE=1;
    fi;


    if [[ "$__DPL_TAG_ELIGIBLE" != "1" ]] && [[ "$DPL_TRIGGER" != "" ]]; then
        echo "Testing for trigger $DPL_TRIGGER, commit message: $TRAVIS_COMMIT_MESSAGE";
        echo "[${DPL_TRIGGER}]";
        echo "[${DPL_TRIGGER}-${_DPL_JOB_NAME}]";
        echo "[${DPL_TRIGGER}-${TRAVIS_OS_NAME}]";
    fi;





    if [[ "$__DPL_TAG_ELIGIBLE" == "1" ]] || [[ "$DPL_TRIGGER" == "" ]] \
    || [[ $TRAVIS_COMMIT_MESSAGE == *"[$DPL_TRIGGER]"* ]] \
    || [[ $TRAVIS_COMMIT_MESSAGE == *"[${DPL_TRIGGER}-${_DPL_JOB_NAME}]"* ]] \
    || [[ $TRAVIS_COMMIT_MESSAGE == *"[${DPL_TRIGGER}-${TRAVIS_OS_NAME}]"* ]]; then




        if [[ "$__DPL_TAG_ELIGIBLE" == "1" ]] || [[ "$DPL_BRANCHES" == "" ]] || [[ $DPL_BRANCHES == *"$TRAVIS_BRANCH"* ]]; then






            if [[ "$__DPL_TERMINATE_EARLY_ELIGIBLE" == "1" ]] && [[ "$DPL_TERMINATE_TESTS" == "1" ]]; then
                __DPL_TRY_TERMINATE_EARLY=1;
            fi;




            if [[ "$DPL_JOB_ENABLE_ALL" == "1" ]] || [[ "$_DPL_JOB_ENABLED" == "1" ]]; then



                if [[ "$DPL_JOBNAMES" == "" ]] || [[ "$_DPL_JOB_NAME" == "" ]] || [[ $DPL_JOBNAMES == *"$_DPL_JOB_NAME"* ]]; then



                    if [[ "$DPL_OSNAMES" == "" ]] || [[ $DPL_OSNAMES == *"$TRAVIS_OS_NAME"* ]]; then



                        if [[ "$_DPL_PACKAGE_BINARY" == "1" ]] || [[ "$_DPL_PACKAGE_SOURCE" == "1" ]]; then
                            if [[ "$_DPL_PACKAGE_MAIN" == "1" ]] || [[ "$_DPL_PACKAGE_ASSET" == "1" ]]; then
                                if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
                                    __DPL_DEBIAN_ACTIVE=1;
                                fi;
                            fi;
                        fi;


                        if [[ "$_DPL_FTP_TARGET" == "1" ]] && [[ "$DPL_FTP_HOSTNAME" != "" ]]; then
                            if [[ "$TRAVIS_OS_HOST" == "linux" ]] && [[ "$DPL_FTP_PROTOCOL" == "ftp" ]]; then
                                echo "Non-secure FTP will not work on Linux Travis-CI jobs!";
                                echo "Try SFTP or another target. Details:";
                                echo "https://blog.travis-ci.com/2018-07-23-the-tale-of-ftp-at-travis-ci";
                            else
                                if [[ "$__DPL_DEBIAN_ACTIVE" == "1" ]] || [[ "$_DPL_PACKAGE_BINARY" == "1" ]] || [[ "$_DPL_BINARY" == "1" ]]; then
                                    echo "Deployer FTP target is enabled";
                                    __DPL_FTP_ACTIVE=1;
                                else
                                    echo "Deployer FTP target cannot be enabled: You must specify _DPL_PACKAGE_BINARY=1,";
                                    echo "and/or _DPL_BINARY=1 in your job's environment variables.";
                                fi;
                            fi;
                        fi;

                        if [[ "$_DPL_DPUT_TARGET" == "1" ]] && [[ "$__DPL_DEBIAN_ACTIVE" == "1" ]] \
                        && [[ "$DPL_DPUT_INCOMING" != "" ]]; then
                            if [[ "$DPL_DPUT_METHOD" == "ftp" ]]; then
                                echo "DPUT will not work with non-secure FTP on Linux Travis-CI jobs!";
                                echo "Try SFTP or another method for DPUT. Details:";
                                echo "https://blog.travis-ci.com/2018-07-23-the-tale-of-ftp-at-travis-ci";
                            else
                                echo "Deployer DPUT target is enabled";
                                __DPL_DPUT_ACTIVE=1;
                            fi;
                        fi;


                        if [[ "$__DPL_FTP_ACTIVE" == "1" ]] || [[ "$__DPL_DPUT_ACTIVE" == "1" ]]; then
                            __DPL_ACTIVE=1;
                        fi;
                    fi;
                fi;
            fi;
        fi;
    else
        if [[ "$DPL_TRIGGER" != "" ]]; then
            echo "Testing for global trigger [$DPL_TRIGGER, commit message: $TRAVIS_COMMIT_MESSAGE";
        fi;
        if [[ "$DPL_TRIGGER" != "" ]] && [[ $TRAVIS_COMMIT_MESSAGE == *"[$DPL_TRIGGER"* ]]; then
            if [[ "$__DPL_TAG_ELIGIBLE" == "1" ]] || [[ "$DPL_BRANCHES" == "" ]] || [[ $DPL_BRANCHES == *"$TRAVIS_BRANCH"* ]]; then


                if [[ "$__DPL_TERMINATE_EARLY_ELIGIBLE" == "1" ]] && [[ "$DPL_TERMINATE_TESTS" == "1" ]]; then

                    __DPL_TRY_TERMINATE_EARLY=1;
                fi;
            fi;
        fi;
    fi;
fi;

if [[ "$__DPL_TRY_TERMINATE_EARLY" == "1" ]] && [[ "$__DPL_ACTIVE" != "1" ]]; then
    echo "Deployer is active in another job";
fi;

if [[ "$__DPL_TRY_TERMINATE_EARLY" != "1" ]] && [[ "$__DPL_ACTIVE" != "1" ]]; then
    echo "Deployer is not active";
fi;
