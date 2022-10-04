#!/bin/bash
#


PROTOCOL="TCP"
PRIORITY="99"
SERVER="lscs_tstsrv"
CLIENT="rtc_tstcli"
SIM_HOSTS="segment-sim node-sim sector-sim"
SEG_HOST="SegA1"
RTC_HOST="glc"
CFG_FILE="~/config/sectorA-F.conf"

TARGET_BINDIR="~/bin/"
BUILD_DIR="~/m1cs-net-test/bin"
BUILD_DIR=`pwd`
STOP="0"
START="0"

usage () {
  echo "Usage: $0 [-f hostfile] [-p priority] [-s lscs_server] [-c rtc_client]"
}

install() {
  echo "Copying binaries to simulators"
  # distribute binaries
  scp -q ${BUILD_DIR}"/am64x/"${SERVER} root@${SEG_HOST}:${TARGET_BINDIR}
  ssh -q root@${SEG_HOST} "ls -l ${TARGET_BINDIR}/${SERVER}"

  for i in ${SIM_HOSTS}; do
    scp -q ${BUILD_DIR}/${SERVER} $i:${TARGET_BINDIR}
    ssh -q $i "ls -l ${TARGET_BINDIR}/${SERVER}"
  done
}

startup () {

  install

  echo "Restarting test servers ..."
  # start lscs servers
  echo "Starting server ${TARGET_BINDIR}/${SERVER}"
  ssh -q -n -f root@${SEG_HOST} "sh -c 'chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -s app_srv19 > /dev/null 2>&1 &'"
    
  for i in 19 18 17 16 15 ; do
    ssh -q -n -f segment-sim "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -s app_srv$i > /dev/null 2>&1 &'"
  done

  for i in 19 18 17 16 15 14 ; do
    ssh -q -n -f node-sim "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -s app_srv$i  > /dev/null 2>&1 &'"
  done

  for i in 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 ; do
    ssh -q -n -f sector-sim "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -s app_srv$i  > /dev/null 2>&1 &'"
  done

  # start the RTC
  echo "Starting client ${BUILD_DIR}/${CLIENT}"

  echo sudo chrt ${PRIORITY} ${BUILD_DIR}/${CLIENT} -t ${PRIORITY} -f ${CFG_FILE}
  sudo ${BUILD_DIR}/${CLIENT} -t ${PRIORITY} -f ${CFG_FILE}
}

startup_udp () {

  install

  echo "Restarting test servers ..."
  # start the RTC
  echo "Starting client ${BUILD_DIR}/${CLIENT}"
  echo sudo "${BUILD_DIR}/${CLIENT} -t ${PRIORITY} -p 30001 30492 2>&1 &"
  
  # start lscs servers
  echo "Starting server ${TARGET_BINDIR}/${SERVER}"
  echo ssh -q -n -f root@${SEG_HOST} "sh -c 'chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -h 10.0.0.1 -p 30001 -c 10.0.1.1 -n 1  > /dev/null 2>&1 &'"
  ssh -q -n -f root@${SEG_HOST} "sh -c 'chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -h 10.0.0.1 -p 30001 -c 10.0.1.1 -n 1  > /dev/null 2>&1 &'"

  #for i in  2 3 4 5 6 ; do
    echo ssh -q -n -f segment-sim "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -h 10.0.0.1 -p 30002 -c 10.0.1.2 -n 5  > /dev/null 2>&1 &'"
    ssh -q -n -f segment-sim "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -h 10.0.0.1 -p 30002 -c 10.0.1.2 -n 5  > /dev/null 2>&1 &'"
  #done

  for i in 07 26 45 64 ; do
    ip=`expr $i + 0`
    echo ssh -q -n -f node-sim "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -h 10.0.0.1 -p 300$i -c 10.0.1.$ip -n 19  > /dev/null 2>&1 &'"
    ssh -q -n -f node-sim "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -h 10.0.0.1 -p 300$i -n 19 -c 10.0.1.$ip > /dev/null 2>&1 &'"
  done
  
  last_ip=0
  sector_ip=2
  for i in 082 107 132 157 182 207 232 257 282 307 332 357 382 407 432 457 ; do
    ip=`expr $i % 82 + 1`
    if [ "$ip" -lt "$last_ip" ] ; then
      sector_ip=`expr $sector_ip + 1`
    fi
    echo ssh -q -n -f sector-sim "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -h 10.0.0.1 -p 30$i -n 25 -c 10.0.$sector_ip"."$ip > /dev/null 2>&1 &'"
    ssh -q -n -f sector-sim "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -h 10.0.0.1 -p 30$i -n 25 -c 10.0.$sector_ip"."$ip > /dev/null 2>&1 &'"
    last_ip=`expr $ip`
  done

  i=482
  ip=`expr $i % 82 + 1`
  echo ssh -q -n -f sector-sim "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -h 10.0.0.1 -p 30482 -n 10 -c 10.0.$sector_ip"."$ip > /dev/null 2>&1 &'"
  ssh -q -n -f sector-sim "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -h 10.0.0.1 -p 30482 -n 10 -c 10.0.$sector_ip"."$ip > /dev/null 2>&1 &'"

  # start the RTC
  echo "Starting client ${BUILD_DIR}/${CLIENT}"
  echo sudo "${BUILD_DIR}/${CLIENT} -t ${PRIORITY} -p 30001 30492 2>&1 &"
}

stop () {
  echo "Stopping test servers ..."
  for i in ${SIM_HOSTS} "root@"${SEG_HOST}; do
    ssh -q -n -f $i "sudo killall ${SERVER}"
  done
}


while [[ $# -gt 0 ]]; do

  case $1 in
    --stop)
      STOP="1"
      shift
      ;;
    --start)
      STOP="1"
      START="1"
      shift
      ;;
    --tcp)
      PROTOCOL="TCP"
      shift
      ;;
    --udp)
      PROTOCOL="UDP"
      SERVER="lscs_udp"
      CLIENT="rtc_udp"
      shift
      ;;
    -f|--file)
      CFG_FILE=$2
      shift
      shift
      ;;
    -p|--priority)
      PRIORITY=$2
      shift
      shift
      ;;
    -*|--*)
      usage
      exit 1
      ;;
  esac
done

if [ "${STOP}" -eq "1" ] ; then
  stop;
fi


if [ "${START}" -eq "1" ] ; then
  if [ "${PROTOCOL}" == "TCP" ] ; then
    startup
  else 
    startup_udp
  fi
fi


