#!/bin/sh













TARGETLIB='ZLIB'                 
STATBNDDIR='ZLIB_A'              
DYNBNDDIR='ZLIB'                 
SRVPGM="ZLIB"                    
IFSDIR='/zlib'                   
TGTCCSID='500'                   
DEBUG='*NONE'                    
OPTIMIZE='40'                    
OUTPUT='*NONE'                   
TGTRLS='V6R1M0'                  

export TARGETLIB STATBNDDIR DYNBNDDIR SRVPGM IFSDIR
export TGTCCSID DEBUG OPTIMIZE OUTPUT TGTRLS








LIBIFSNAME="/QSYS.LIB/${TARGETLIB}.LIB"















action_needed()

{
        [ ! -e "${1}" ] && return 0
        [ "${2}" ] || return 1
        [ "${1}" -ot "${2}" ] && return 0
        return 1
}








make_module()

{
    MODULES="${MODULES} ${1}"
    MODIFSNAME="${LIBIFSNAME}/${1}.MODULE"
    CSRC="`basename \"${2}\"`"

    if action_needed "${MODIFSNAME}" "${2}"
    then    :
    elif [ ! "`sed -e \"/<source name=\\\"${CSRC}\\\">/,/<\\\\/source>/!d\" \
      -e '/<depend /!d'                                                 \
      -e 's/.* name=\"\\([^\"]*\\)\".*/\\1/' < \"${TOPDIR}/treebuild.xml\" |
        while read HDR
        do      if action_needed \"${MODIFSNAME}\" \"${IFSDIR}/include/${HDR}\"
                then    echo recompile
                        break
                fi
        done`" ]
    then    return 0
    fi

    CMD="CRTCMOD MODULE(${TARGETLIB}/${1}) SRCSTMF('${2}')"
    CMD="${CMD} SYSIFCOPT(*IFS64IO) OPTION(*INCDIRFIRST)"
    CMD="${CMD} LOCALETYPE(*LOCALE) FLAG(10)"
    CMD="${CMD} INCDIR('${IFSDIR}/include' ${INCLUDES})"
    CMD="${CMD} TGTCCSID(${TGTCCSID}) TGTRLS(${TGTRLS})"
    CMD="${CMD} OUTPUT(${OUTPUT})"
    CMD="${CMD} OPTIMIZE(${OPTIMIZE})"
    CMD="${CMD} DBGVIEW(${DEBUG})"
    system "${CMD}"
    LINK=YES
}




db2_name()

{
        basename "${1}"                                                 |
        tr 'a-z-' 'A-Z_'                                                |
        sed -e 's/\..*//'                                               \
            -e 's/^\(.\).*\(.........\)$/\1\2/'
}




copy_hfile()

{
        sed -e '1i\
#pragma enum(int)\
' "${@}" -e '$a\
#pragma enum(pop)\
'
}








SCRIPTDIR=`dirname "${0}"`

case "${SCRIPTDIR}" in
/*)     ;;
*)      SCRIPTDIR="`pwd`/${SCRIPTDIR}"
esac

while true
do      case "${SCRIPTDIR}" in
        */.)    SCRIPTDIR="${SCRIPTDIR%/.}";;
        *)      break;;
        esac
done



TOPDIR=`dirname "${SCRIPTDIR}"`
export SCRIPTDIR TOPDIR
cd "${TOPDIR}"




VERSION=`sed -e '/^<package /!d'                                        \
            -e 's/^.* version="\([0-9.]*\)".*$/\1/' -e 'q'              \
            < treebuild.xml`
export VERSION






if action_needed "${LIBIFSNAME}"
then    CMD="CRTLIB LIB(${TARGETLIB}) TEXT('ZLIB: Data compression API')"
        system "${CMD}"
fi




if action_needed "${LIBIFSNAME}/DOCS.FILE"
then    CMD="CRTSRCPF FILE(${TARGETLIB}/DOCS) RCDLEN(112)"
        CMD="${CMD} CCSID(${TGTCCSID}) TEXT('Documentation texts')"
        system "${CMD}"
fi



for TEXT in "${TOPDIR}/ChangeLog" "${TOPDIR}/FAQ"                       \
    "${TOPDIR}/README" "${SCRIPTDIR}/README400"
do      MEMBER="${LIBIFSNAME}/DOCS.FILE/`db2_name \"${TEXT}\"`.MBR"

        if action_needed "${MEMBER}" "${TEXT}"
        then    CMD="CPY OBJ('${TEXT}') TOOBJ('${MEMBER}') TOCCSID(${TGTCCSID})"
                CMD="${CMD} DTAFMT(*TEXT) REPLACE(*YES)"
                system "${CMD}"
        fi
done




SRCPF="${LIBIFSNAME}/H.FILE"

if action_needed "${SRCPF}"
then    CMD="CRTSRCPF FILE(${TARGETLIB}/H) RCDLEN(112)"
        CMD="${CMD} CCSID(${TGTCCSID}) TEXT('ZLIB: C/C++ header files')"
        system "${CMD}"
fi




if action_needed "${IFSDIR}/include"
then    mkdir -p "${IFSDIR}/include"
fi



for HFILE in "${TOPDIR}/"*.h
do      DEST="${SRCPF}/`db2_name \"${HFILE}\"`.MBR"

        if action_needed "${DEST}" "${HFILE}"
        then    copy_hfile < "${HFILE}" > tmphdrfile



                CMD="CPY OBJ('`pwd`/tmphdrfile') TOOBJ('${DEST}')"
                CMD="${CMD} TOCCSID(${TGTCCSID}) DTAFMT(*TEXT) REPLACE(*YES)"
                system "${CMD}"

                rm -f tmphdrfile
        fi

        IFSFILE="${IFSDIR}/include/`basename \"${HFILE}\"`"

        if action_needed "${IFSFILE}" "${DEST}"
        then    rm -f "${IFSFILE}"
                ln -s "${DEST}" "${IFSFILE}"
        fi
done





HFILE="${SCRIPTDIR}/zlib.inc"
DEST="${SRCPF}/ZLIB.INC.MBR"

if action_needed "${DEST}" "${HFILE}"
then    CMD="CPY OBJ('${HFILE}') TOOBJ('${DEST}')"
        CMD="${CMD} TOCCSID(${TGTCCSID}) DTAFMT(*TEXT) REPLACE(*YES)"
        system "${CMD}"

fi

IFSFILE="${IFSDIR}/include/`basename \"${HFILE}\"`"

if action_needed "${IFSFILE}" "${DEST}"
then    rm -f "${IFSFILE}"
        ln -s "${DEST}" "${IFSFILE}"
fi




echo '#pragma comment(user, "ZLIB version '"${VERSION}"'")' > os400.c
echo '#pragma comment(user, __DATE__)' >> os400.c
echo '#pragma comment(user, __TIME__)' >> os400.c
echo '#pragma comment(copyright, "Copyright (C) 1995-2017 Jean-Loup Gailly, Mark Adler. OS/400 version by P. Monnerat.")' >> os400.c
make_module     OS400           os400.c
LINK=                            
MODULES=




CSOURCES=`sed -e '/<source name="/!d'                                   \
    -e 's/.* name="\([^"]*\)".*/\1/' < treebuild.xml`



for SRC in ${CSOURCES}
do      MODULE=`db2_name "${SRC}"`
        make_module "${MODULE}" "${SRC}"
done




if action_needed "${LIBIFSNAME}/${STATBNDDIR}.BNDDIR"
then    LINK=YES
fi

if [ "${LINK}" ]
then    rm -rf "${LIBIFSNAME}/${STATBNDDIR}.BNDDIR"
        CMD="CRTBNDDIR BNDDIR(${TARGETLIB}/${STATBNDDIR})"
        CMD="${CMD} TEXT('ZLIB static binding directory')"
        system "${CMD}"

        for MODULE in ${MODULES}
        do      CMD="ADDBNDDIRE BNDDIR(${TARGETLIB}/${STATBNDDIR})"
                CMD="${CMD} OBJ((${TARGETLIB}/${MODULE} *MODULE))"
                system "${CMD}"
        done
fi





if action_needed "${LIBIFSNAME}/TOOLS.FILE"
then    CMD="CRTSRCPF FILE(${TARGETLIB}/TOOLS) RCDLEN(112)"
        CMD="${CMD} CCSID(${TGTCCSID}) TEXT('ZLIB: build tools')"
        system "${CMD}"
fi


DEST="${LIBIFSNAME}/TOOLS.FILE/BNDSRC.MBR"

if action_needed "${SCRIPTDIR}/bndsrc" "${DEST}"
then    CMD="CPY OBJ('${SCRIPTDIR}/bndsrc') TOOBJ('${DEST}')"
        CMD="${CMD} TOCCSID(${TGTCCSID}) DTAFMT(*TEXT) REPLACE(*YES)"
        system "${CMD}"

        LINK=YES
fi




if action_needed "${LIBIFSNAME}/${SRVPGM}.SRVPGM"
then    LINK=YES
fi

if [ "${LINK}" ]
then    CMD="CRTSRVPGM SRVPGM(${TARGETLIB}/${SRVPGM})"
        CMD="${CMD} SRCFILE(${TARGETLIB}/TOOLS) SRCMBR(BNDSRC)"
        CMD="${CMD} MODULE(${TARGETLIB}/OS400)"
        CMD="${CMD} BNDDIR(${TARGETLIB}/${STATBNDDIR})"
        CMD="${CMD} TEXT('ZLIB ${VERSION} dynamic library')"
        CMD="${CMD} TGTRLS(${TGTRLS})"
        system "${CMD}"
        LINK=YES



        BACKUP=`echo "${SRVPGM}${VERSION}"                              |
                sed -e 's/.*\(..........\)$/\1/' -e 's/\./_/g'`
        BACKUP="`db2_name \"${BACKUP}\"`"
        BKUPIFSNAME="${LIBIFSNAME}/${BACKUP}.SRVPGM"
        rm -f "${BKUPIFSNAME}"
        CMD="CRTDUPOBJ OBJ(${SRVPGM}) FROMLIB(${TARGETLIB})"
        CMD="${CMD} OBJTYPE(*SRVPGM) NEWOBJ(${BACKUP})"
        system "${CMD}"
fi




if action_needed "${LIBIFSNAME}/${DYNBNDDIR}.BNDDIR"
then    LINK=YES
fi

if [ "${LINK}" ]
then    rm -rf "${LIBIFSNAME}/${DYNBNDDIR}.BNDDIR"
        CMD="CRTBNDDIR BNDDIR(${TARGETLIB}/${DYNBNDDIR})"
        CMD="${CMD} TEXT('ZLIB dynamic binding directory')"
        system "${CMD}"
        CMD="ADDBNDDIRE BNDDIR(${TARGETLIB}/${DYNBNDDIR})"
        CMD="${CMD} OBJ((*LIBL/${SRVPGM} *SRVPGM))"
        system "${CMD}"
fi
