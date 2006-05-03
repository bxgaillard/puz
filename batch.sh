#!/bin/bash

if [ $# -ne 3 ]; then
    echo "Syntax: $0 <smp|bsub> <matrix_file> <time> [args to puz...]" 1>&2
    exit 1
fi

if [ "x$1" != xsmp -a "x$1" != xbsub ]; then
    echo "Error: $1 should be either 'smp' or 'bsub'." 1>&2
    exit 2
fi

if [ ! -r "$2" ]; then
    echo "Error: $2 cannot be read." 1>&2
    exit 2
fi

METHOD="$1"
FILE="$2"
TIME="$3"
EXE_DIR=exe
OUT_DIR=output
EXE_BASE="${EXE_DIR}/puz"
EXE_SUFFIX=
OUT_BASE="${OUT_DIR}/puz"
OUT_SUFFIX=.txt

shift; shift; shift

[ ! -d "${EXE_DIR}" ] && mkdir -p "${EXE_DIR}"
[ ! -d "${OUT_DIR}" ] && mkdir -p "${OUT_DIR}"

for ((NPROC=1; NPROC<=16; NPROC++)); do
    EXE="${EXE_BASE}-${NPROC}${EXE_SUFFIX}"
    OUT="${OUT_BASE}-${NPROC}${OUT_SUFFIX}"

    echo "Compiling for ${NPROC} processor(s)..."
    rm -f "${EXE}"
    make clean
    make UPCTHREADS="${NPROC}" UPC_TARGET="${EXE}" "${EXE}"

    echo "Dispatching on ${NPROC} processor(s) (queue '${QUEUE}')..."
    if [ "${METHOD}" = bsub ]; then
	[ "${NPROC}" -le 4 ] && QUEUE=courts || QUEUE=production
	bsub -q "${QUEUE}" -n "${NPROC}" -oo "${OUT}" -W "${TIME}" \
	    -a mpich_gm mpirun.lsf "${EXE}" -i "${FILE}" "$@"
    else
	"${EXE}" -i "${FILE}" "$@" &> "${OUT}"
    fi

#    echo 'Waiting for output file...'
#    while [ ! -f "${OUT}" ]; do
#	sleep 1
#    done
#
#    set $(grep ^Time "${OUT}" | sed 's/^Time \?: //')
#    #rm -f "${PROG}" "${OUT}"
#    nb="$#"
#    sum="$1"
#    shift
#
#    while [ $# -ne 0 ]; do
#	sum="${sum} + $1"
#	shift
#    done
#
#    mtime=$(echo "scale = 6; (${sum}) / ${nb}" | bc -q)
#
#    echo "Mean time for ${NPROC} processor(s): ${mtime} s"
done
