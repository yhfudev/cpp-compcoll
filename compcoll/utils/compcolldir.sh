#!/bin/bash
#####################################################################
# compare the files in two directories
#
# You need to prepare the original files and the new files in two seperated directories.
# the original files are placed in directory '1', and the new files are placed in '2'.
# the compared results are in directory 'results'
#
# Copyright 2014 Yunhui Fu
# License: GPL v3.0 or later
#####################################################################
my_getpath () {
  PARAM_DN="$1"
  shift
  #readlink -f
  DN="${PARAM_DN}"
  FN=
  if [ ! -d "${DN}" ]; then
    FN=$(basename "${DN}")
    DN=$(dirname "${DN}")
  fi
  cd "${DN}" > /dev/null 2>&1
  DN=$(pwd)
  cd - > /dev/null 2>&1
  if [ "${FN}" = "" ]; then
    echo "${DN}"
  else
    echo "${DN}/${FN}"
  fi
}
#DN_EXEC=`echo "$0" | ${EXEC_AWK} -F/ '{b=$1; for (i=2; i < NF; i ++) {b=b "/" $(i)}; print b}'`
#echo "[DBG] 0=$0" 1>&2
DN_EXEC=$(dirname $(my_getpath "$0") )
if [ ! "${DN_EXEC}" = "" ]; then
    DN_EXEC="$(my_getpath "${DN_EXEC}")/"
else
    DN_EXEC="${DN_EXEC}/"
fi

#####################################################################
uuid() {
    local N B C='89ab'

    for (( N=0; N < 16; ++N )) ; do
        B=$(( $RANDOM%256 ))
        case $N in
        6)
            printf '4%x' $(( B%16 ))
            ;;
        8)
            printf '%c%x' ${C:$RANDOM%${#C}:1} $(( B%16 ))
            ;;
        3 | 5 | 7 | 9)
            printf '%02x-' $B
            ;;
        *)
            printf '%02x' $B
            ;;
        esac
    done
}

#####################################################################
FN_ERR=/dev/stderr

# 是否输出到单独一个文件
FLG_SINGLE_FILE=0

# list and sort the files in two directories
DN_ORIG=
DN_NEW=
DN_OUT=

PRGNAME=$0

PREFIX0=""

TMPID=$(uuid)
FN_LST_ORIG="cctmplst1-${TMPID}"
FN_LST_NEW="cctmplst2-${TMPID}"

#####################################################################

usage () {
  PARAM_PRGNAME="$1"

  echo "" >> "/dev/stderr"
  echo "Compare the files in two folders" >> "/dev/stderr"
  echo "Written by yhfu, 2014-09" >> "/dev/stderr"
  echo "" >> "/dev/stderr"
  echo "${PARAM_PRGNAME} [options] [<old dir> [<new dir> [<out dir>]]] " >> "/dev/stderr"
  echo "" >> "/dev/stderr"
  echo "Options:" >> "/dev/stderr"
  echo -e "\t--help|-h                     Print this message" >> "/dev/stderr"
  echo -e "\t--prefix|-p <PREFIX>          Use the user's PREFIX as the prefix of data name (default: ${PREFIX0})" >> "/dev/stderr"
  echo -e "\t--mergesame|-m                Merge adjacent changes" >> "/dev/stderr"
  echo -e "\t--outreturn|-r <old|new|all>  Output the 'return'" >> "/dev/stderr"
  echo -e "\t--singleout|-s                Use single output file" >> "/dev/stderr"
  echo "" >> "/dev/stderr"
  echo "Examples:" >> "/dev/stderr"
  echo "  ${PARAM_PRGNAME} -h        # Print this message." >> "/dev/stderr"
}
OTHER_OPT=
CNT=0
while [ ! "$1" = "" ]; do
    case "$1" in
    --help|-h)
        usage "${PRGNAME}"
        exit 0
        ;;
    --prefix|-p)
        shift
        PREFIX0="$1"
        ;;
    --singleout|-s)
        FLG_SINGLE_FILE=1
        ;;
    --mergesame|-m)
        OTHER_OPT="${OTHER_OPT} -m"
        #echo "$0: merge same" >> "${FN_ERR}"
        ;;
    --outreturn|-r)
        shift
        OTHER_OPT="${OTHER_OPT} -r $1"
        ;;
    -*)
        echo "$0: Unknown option: $1" >> "${FN_ERR}"
        echo "Use option --help to get the usages." >> "${FN_ERR}"
        exit 1
        ;;
    *)
        case $CNT in
        0)
            DN_ORIG="$1"
            ;;
        1)
            DN_NEW="$1"
            ;;
        2)
            DN_OUT="$1"
            ;;
        esac
        CNT=$((${CNT} + 1))
        ;;
    esac
    shift
done

if [ "${DN_ORIG}" = "" ]; then
    DN_ORIG=1
fi
if [ "${DN_NEW}" = "" ]; then
    DN_NEW=2
fi
if [ "${DN_OUT}" = "" ]; then
    DN_OUT=results
fi

PREFIX="${DN_OUT}/${PREFIX0}"
SUFFIX=".htm"


#echo "DN_ORIG=${DN_ORIG}"
#echo "DN_NEW=${DN_NEW}"
#echo "DN_OUT=${DN_OUT}"
#echo "prefix=${PREFIX}"
#echo "suffix=${SUFFIX}"
#echo "FN_LST_ORIG=${FN_LST_ORIG}"
#echo "FN_LST_NEW=${FN_LST_NEW}"
#exit 0

#####################################################################
EXEC_COMPCOLL=$(which compcoll)
if [ "${EXEC_COMPCOLL}" = "" ]; then
    EXEC_COMPCOLL="../src/compcoll"
fi


find ${DN_ORIG} -type f | sort > ${FN_LST_ORIG}
find ${DN_NEW}  -type f | sort > ${FN_LST_NEW}

CNT=1

rm -rf "${DN_OUT}"
mkdir -p "${DN_OUT}"

if [ "${FLG_SINGLE_FILE}" = "1" ]; then
    FN_OUT="${PREFIX}results${SUFFIX}"
    ${EXEC_COMPCOLL} -H > "${FN_OUT}"
fi

# read from two files
while true; do
    read -r LN_ORIG <&3
    read -r LN_NEW  <&4
    if [ -z "$LN_ORIG" -o -z "$LN_NEW" ]; then
        break
    fi
    if [ ! "${FLG_SINGLE_FILE}" = "1" ]; then
        FN_OUT="${PREFIX}$(echo $CNT | awk '{printf ("%019d", $1);}')${SUFFIX}"
    fi
    echo "=====comparing [${CNT}] ..."
    echo "1: "${LN_ORIG}
    echo "2: "${LN_NEW}
    echo "out: ${FN_OUT}"

    if [ "${FLG_SINGLE_FILE}" = "1" ]; then
        ${EXEC_COMPCOLL} ${OTHER_OPT} -x ${CNT} -C "$LN_ORIG" "$LN_NEW" >> "${FN_OUT}"
    else
        ${EXEC_COMPCOLL} ${OTHER_OPT} -x ${CNT} "$LN_ORIG" "$LN_NEW" > "${FN_OUT}"
    fi
    CNT=$(($CNT + 1))
done 3<${FN_LST_ORIG} 4<${FN_LST_NEW}

if [ "${FLG_SINGLE_FILE}" = "1" ]; then
    ${EXEC_COMPCOLL} -T >> "${FN_OUT}"
fi

rm -f ${FN_LST_ORIG} ${FN_LST_NEW}

echo "=========================== DONE!"
