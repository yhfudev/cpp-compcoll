#!/bin/bash
#####################################################################
# split the story from (HTML/TEXT) format by '第x回'
#
# The script will detect the encoding and convert it to utf-8 text files.
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
#EXEC_W3M=html2text.py
EXEC_W3M0=$(which w3m)
if [ "${EXEC_W3M0}" = "" ]; then
    EXEC_W3M0="../src/w3m"
fi
EXEC_W3M="${EXEC_W3M0} -M -dump"

EXEC_HTM2TXT=""
if [ ! -x "${EXEC_HTM2TXT}" ]; then
    EXEC_HTM2TXT="./htm2txt.sh"
fi

EXEC_UCDET=$(which ucdet)
if [ "${EXEC_UCDET}" = "" ]; then
    EXEC_UCDET="../src/ucdet"
fi

DN_ORIG=
DN_OUT=

PRGNAME=$0

PREFIX0=""

TMPID=$(uuid)
FN_LST_ORIG="splittmplst1-${TMPID}"

#####################################################################

usage () {
  PARAM_PRGNAME="$1"

  echo "" >> "/dev/stderr"
  echo "Split a file to multiple segment files by token string" >> "/dev/stderr"
  echo "Written by yhfu, 2014-09" >> "/dev/stderr"
  echo "" >> "/dev/stderr"
  echo "${PARAM_PRGNAME} [options] [<input dir> [<out dir>]] " >> "/dev/stderr"
  echo "" >> "/dev/stderr"
  echo "Options:" >> "/dev/stderr"
  echo -e "\t--help|-h                     Print this message" >> "/dev/stderr"
  echo -e "\t--prefix|-p <PREFIX>          Use the user's PREFIX as the prefix of data name (default: ${PREFIX0})" >> "/dev/stderr"
  echo -e "\t--token|-t  <TOKEN>           The token to split the contents (example: 'Chapter[\w](.*)', '　第(.*)囘')" >> "/dev/stderr"
  echo "" >> "/dev/stderr"
  echo "Examples:" >> "/dev/stderr"
  echo "  ${PARAM_PRGNAME} -h        # Print this message." >> "/dev/stderr"
}

TOKEN=
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
    --token|-t)
        shift
        TOKEN="$1"
        ;;
    -*)
        echo "Unknown option: $1"
        echo "Use option --help to get the usages."
        exit 1
        ;;
    *)
        case $CNT in
        0)
            DN_ORIG="$1"
            ;;
        1)
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
if [ "${DN_OUT}" = "" ]; then
    DN_OUT=results
fi

PREFIX="${DN_OUT}/${PREFIX0}"
SUFFIX=".txt"

echo "DN_ORIG=${DN_ORIG}"
echo "DN_OUT=${DN_OUT}"
echo "prefix=${PREFIX}"
echo "suffix=${SUFFIX}"
echo "TOKEN=${TOKEN}"
#echo "FN_LST_ORIG=${FN_LST_ORIG}"
#echo "FN_LST_NEW=${FN_LST_NEW}"
#exit 0


find ${DN_ORIG} -type f | sort > ${FN_LST_ORIG}

rm -rf "${DN_OUT}"
mkdir -p "${DN_OUT}"

# read from two files
CNT=0
while true; do
    read -r LN_ORIG <&3
    if [ -z "$LN_ORIG" ]; then
        break
    fi
    echo "orig: "${LN_ORIG}

    FN_TMP=
    LN_ORIG2="${LN_ORIG}"
    # detect the encoding
    ENC=$(${EXEC_UCDET} "${LN_ORIG}")
    echo ${ENC} | grep -i "utf8"
    if [ ! $? = 0 ]; then
        echo ${ENC} | grep -i "utf-8"
        if [ ! $? = 0 ]; then
            FN_TMP="$(dirname ${PREFIX})/mytmp-$(basename ${LN_ORIG})"
            iconv -c -f ${ENC} -t UTF-8 "${LN_ORIG}" > "${FN_TMP}"
            LN_ORIG2="${FN_TMP}"
        fi
    fi
    #echo "enc=${ENC}"; exit 1

    case "${LN_ORIG2}" in
    *.html|*.htm)
        FN_TMP="$(dirname ${PREFIX})/mytmp-2-$(basename ${LN_ORIG})"
        $EXEC_HTM2TXT "${LN_ORIG2}"   \
            | grep -v '国学导航'    \
            | grep -v 'www.guoxue123.com' \
            | grep -v 'Copyright' \
            | grep -v 'jwqmxxy'   \
            | grep -v '下一页'     \
            | grep -v '┐'         \
            | grep -v '┘'         \
            | grep -v 'Powered by' \
            | grep -v '查看完整版本'  \
            | grep -v '頁: '        \
            | grep -v '龍壇書網'     \
            | sed -e 's/│//g'     \
                  -e 's|[ ]*$||g' \
                  -e 's|^[ ]*||g' \
                  -e 's|　*$||g'   \
            > "${FN_TMP}"

        LN_ORIG2="${FN_TMP}"
        ;;
    *.txt)
        echo "skip txt"
        ;;
    *)
        echo "not support file type: ${LN_ORIG}"
        continue
        ;;
    esac

    FN_SRC="${LN_ORIG2}"
    echo "process file: ${FN_SRC}"
    if [ "${TOKEN}" = "" ]; then
        PAT1="[　]{2}第([一二三四五六七八九十百]*)[囘回囬]"
        echo "try pattern: '${PAT1}' ..."
        head -n 1600 "${FN_SRC}" | grep -E "${PAT1}"
        if [ ! "$?" = "0" ]; then
            PAT1="第(.*)回　"
            echo "try pattern: '${PAT1}' ..."
            head -n 1600 "${FN_SRC}" | grep -E "${PAT1}"
        fi
        if [ ! "$?" = "0" ]; then
            PAT1="　第(.*)囘"
            echo "try pattern: '${PAT1}' ..."
            head -n 1600 "${FN_SRC}" | grep -E "${PAT1}"
        fi
        if [ ! "$?" = "0" ]; then
            PAT1="第(.*)回《"
            echo "try pattern: '${PAT1}' ..."
            head -n 1600 "${FN_SRC}" | grep -E "${PAT1}"
        fi
        if [ ! "$?" = "0" ]; then
            PAT1="\xc第(.*)回"
            echo "try pattern: '${PAT1}' ..."
            head -n 1600 "${FN_SRC}" | grep -E "${PAT1}"
        fi
        if [ ! "$?" = "0" ]; then
            PAT1="第.*回 "
            echo "try pattern: '${PAT1}' ..."
            head -n 1600 "${FN_SRC}" | grep -E "${PAT1}"
        fi
        if [ ! "$?" = "0" ]; then
            PAT1="第.*囘"
            echo "try pattern: '${PAT1}' ..."
            head -n 1600 "${FN_SRC}" | grep -E "${PAT1}"
        fi
        if [ ! "$?" = "0" ]; then
            PAT1="第.*回"
            echo "try pattern: '${PAT1}' ..."
            head -n 1600 "${FN_SRC}" | grep -E "${PAT1}"
        fi
        if [ ! "$?" = "0" ]; then
            PAT1="第.*章"
            echo "try pattern: '${PAT1}' ..."
            head -n 1600 "${FN_SRC}" | grep -E "${PAT1}"
        fi
        cat "${FN_SRC}" | grep -E "${PAT1}"
    else
        PAT1="${TOKEN}"
    fi
    echo "use pattern: '${PAT1}'"
    CNT=$(cat "${FN_SRC}" | awk -v PAT1="${PAT1}" -v CNT=${CNT} -v PREFIX=${PREFIX} -v SUFFIX=${SUFFIX} 'BEGIN{c=CNT; outfile=sprintf(PREFIX "%019d" SUFFIX, c); }{if (match($0, PAT1 )) {c++; outfile=sprintf(PREFIX "%019d" SUFFIX, c); } print $0 >> outfile; }END{print c;}')

    if [ ! "${FN_TMP}" = "" ]; then
        rm -f "$(dirname ${FN_TMP})/mytmp-"*
    fi

    CNT=$(($CNT + 1))

done 3<${FN_LST_ORIG}

rm -f ${FN_LST_ORIG}

