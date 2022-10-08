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

TARGET_BINDIR="~/bin"
BUILD_DIR="~/m1cs-net-test/bin"
BUILD_DIR=`pwd`
STOP="0"
START="0"

date

usage () {
  echo "Usage: $0 [--start | --stop] [--tcp | --udp] [-f hostfile] [-p priority] [-s lscs_server] [-c rtc_client]"
}

install() {
  echo "Installing binaries on simulators"
  # distribute binaries
  scp -q ${BUILD_DIR}"/am64x/"${SERVER} root@${SEG_HOST}:${TARGET_BINDIR}
  ssh -q root@${SEG_HOST} "ls -l ${TARGET_BINDIR}/${SERVER}"

  for i in ${SIM_HOSTS}; do
    scp -q ${BUILD_DIR}/${SERVER} $i:${TARGET_BINDIR}
    ssh -q $i "ls -l ${TARGET_BINDIR}/${SERVER}"
  done
}

# $1 = host to ssh to
# $2 = server name
#
start_lscs_tcp () {
  echo "ssh -q -n -f $1 \"sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -s $2 > /dev/null 2>&1 &'\""
  ssh -q -n -f $1 "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -s $2 > /dev/null 2>&1 &'"
}

startup () {

  install

  echo "Restarting test servers ..."
  # start lscs servers
  echo "Starting server ${TARGET_BINDIR}/${SERVER}"
  start_lscs_tcp "root@${SEG_HOST}" "app_srv19"
  #ssh -q -n -f root@${SEG_HOST} "sh -c 'chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -s app_srv19 > /dev/null 2>&1 &'"
    
  for i in 19 18 17 16 15 ; do
    #ssh -q -n -f segment-sim "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -s app_srv$i > /dev/null 2>&1 &'"
    start_lscs_tcp segment-sim "app_srv$i"
  done

  for i in 19 18 17 16 15 14 ; do
    #ssh -q -n -f node-sim "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -s app_srv$i  > /dev/null 2>&1 &'"
    start_lscs_tcp node-sim "app_srv$i"
  done

  for i in 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 ; do
    #ssh -q -n -f sector-sim "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -s app_srv$i  > /dev/null 2>&1 &'"
    start_lscs_tcp sector-sim "app_srv$i"
  done

  # start the RTC
  echo "Starting client ${BUILD_DIR}/${CLIENT}"

  echo sudo chrt ${PRIORITY} ${BUILD_DIR}/${CLIENT} -t ${PRIORITY} -f ${CFG_FILE}
  #sudo chrt ${PRIORITY} ${BUILD_DIR}/${CLIENT} -t ${PRIORITY} -f ${CFG_FILE} 
  #sudo chrt ${PRIORITY} ${BUILD_DIR}/${CLIENT} -d -t ${PRIORITY} -f ${CFG_FILE} 2>&1 &
}

# $1 = host to ssh to 
# $2 = ip to send data to
# $3 = lscs's ip to simulate
# $4 = base port number to send data to
# $5 = number of ports to send data to

start_lscs_udp () {
    #echo ssh -q -n -f $1 "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -h $2 -c $3 -p $4 -n $5  > /dev/null 2>&1 &'"
    msg="\nssh -q -n -f $1 \"sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -h $2 -c $3 -p $4 -n $5  > /dev/null 2>&1 &'\"\n"
    echo -e $msg
    ssh -q -n -f $1 "sh -c 'sudo chrt ${PRIORITY} ${TARGET_BINDIR}/${SERVER} -h $2 -c $3 -p $4 -n $5  > /dev/null 2>&1 &'"
}

startup_udp () {

  install

  echo "Restarting test servers ..."
  # start the RTC
  echo "Starting client ${BUILD_DIR}/${CLIENT}"
  echo sudo "${BUILD_DIR}/${CLIENT} -t ${PRIORITY} -p 30001 30492 2>&1 &"
  sudo chrt ${PRIORITY} ${BUILD_DIR}/${CLIENT} -t ${PRIORITY} -p 30001 30492 2>&1 &

  # start lscs servers
  echo "Starting server ${TARGET_BINDIR}/${SERVER}"
  start_lscs_udp "root@${SEG_HOST}" "10.0.0.1" "10.0.1.1" 30001 1

  i=2
  while [ $i -le 6 ]
  do
    port=`expr $i + 30000`
    start_lscs_udp segment-sim "10.0.0.$i" "10.0.1.$i" $port 1
    i=`expr $i + 1`
  done

  i=7
  h=1
  while [ $i -le 73 ]
  do
    port=`expr $i + 30000`
    start_lscs_udp node-sim "10.0.0.$h" "10.0.1.$i" $port 6
    i=`expr $i + 6`
    h=`expr $h + 1`
  done
  port=`expr $i + 30000`
  start_lscs_udp node-sim "10.0.0.$h" "10.0.1.$i" $port 4

  i=2
  port=30083
  while [ $i -le 7 ]
  do
    start_lscs_udp sector-sim "10.0.0.$i" "10.0.$i.1" $port 27
    port=`expr $port + 27`
    start_lscs_udp sector-sim "10.0.0.$i" "10.0.$i.28" $port 27
    port=`expr $port + 27`
    start_lscs_udp sector-sim "10.0.0.$i" "10.0.$i.55" $port 28
    port=`expr $port + 28`
    i=`expr $i + 1`
  done
}

stop () {
  echo "Stopping test servers ..."
  for i in ${SIM_HOSTS} "root@${SEG_HOST};" do
    ssh -q -n -f $i "sudo killall ${SERVER}"
  done
  sudo killall ${CLIENT}
}

if [[ $# -eq 0 ]]; then
  usage
  exit 1
fi  

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


