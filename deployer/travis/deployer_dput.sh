#!/bin/bash





if [[ "$__DPL_DPUT_ACTIVE" == "1" ]]; then


    sudo apt-get install python-paramiko expect dput;  




    cat > "./dput.cf" << EOM
[deployer]
fqdn = ${DPL_DPUT_DOMAIN}
method = ${DPL_DPUT_METHOD}
incoming = ${DPL_DPUT_INCOMING}
login = ${DPL_DPUT_LOGIN}
allow_unsigned_uploads = 0
EOM



    cat >> "./ssh_config" << EOM

Host *
    StrictHostKeyChecking no
    UserKnownHostsFile=/dev/null
    PubKeyAuthentication yes
    IdentityFile ${PWD}/key.private
    IdentitiesOnly yes
EOM
    sudo sh -c "cat < ${PWD}/ssh_config >> /etc/ssh/ssh_config";


    echo "$DPL_SSH_KEY_PRIVATE" | base64 --decode > key.private;
    chmod 700 ./key.private;

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
            for f in $n*.changes; do


                if [[ "$_DPL_PACKAGE_SOURCE" != "1" ]]; then
                    if [[ "$f" == *"_source"* ]] || [[ "$f" == *".tar.xz"* ]]; then
                        continue;
                    fi;
                fi;

                expect <(cat <<EOD
spawn dput -c "${OLDPWD}/dput.cf" deployer "$f";
expect "Enter passphrase for key"
send "${DPL_SSH_KEY_PASSPHRASE}\r"
interact
EOD
);
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
            for f in $n*.changes; do


                if [[ "$_DPL_PACKAGE_SOURCE" != "1" ]]; then
                    if [[ "$f" == *"_source"* ]] || [[ "$f" == *".tar.xz"* ]]; then
                        continue;
                    fi;
                fi;
                expect <(cat <<EOD
spawn dput -c "${OLDPWD}/dput.cf" deployer "$f";
expect "Enter passphrase for key"
send "${DPL_SSH_KEY_PASSPHRASE}\r"
interact
EOD
);
            done;
        done;


        cd $OLDPWD;
    fi;

    srm ./key.private;
fi;
