#!/bin/bash

function usage()
{
    echo "USAGE: $1" 
    echo -e "\t-m <module_name>\t必须, 模块目录名" 
    echo -e "\t-c <conf_file>\t\t可选, 模块conf目录下配置文件名, 默认为module.conf"
    echo -e "\t-t <timeout>\t\t可选, 整个作业流的超时时间, 可以为浮点数, 单位为小时"
    echo -e "\t-a <args>\t\t可选, 其他自定义任务启动参数"
    echo -e "\t-h <help>\t\t打印Usage"
}

# TO BE IMPLEMENTED
function send_mail_msg() {}
# TO BE IMPLEMENTED
function send_gsm_msg() {}

function read_warning_level()
{
    source $1
    return 0
}

# 判断timeout参数是否为正浮点数
# 是则返回0, 不是返回1
function check_timeout_val()
{
    local var=$(echo "$1 > 0" | bc 2>/dev/null)
    if [ $var -eq 1 ]; then
        return 0
    else
        return 1
    fi
}

function alarm_timeout()
{
    local start_time="$1"
    local msg="TIMEOUT REMIND: Module [${module_name}] timeout. Pleast check as quickly as possible! Starting time: [${start_time}]"
    send_mail_msg -t 0 -s "$msg" -p "$msg"
    send_gsm_msg -t 0 -s "$msg"
}

function alarm_failure()
{
    local proc_ret="$1"
    local start_time="$2"
    local msg="FAILURE REMIND: Module [${module_name}] failed with ret [${proc_ret}]. Start at[${start_time}]. Pleast check as quickly as possible!"
    send_mail_msg -t 0 -s "$msg" -p "$msg"
    send_gsm_msg -t 0 -s "$msg"
}

function watch_timeout()
{
    local timeout_hours=$1
    local watch_pid=$2
    local ppid=$3
    local running_flag=$4
    local start_watching_time=`date +"%Y-%m-%d %H:%M:%S"`
    
    # check running flag file exists
    if [ ! -f "${running_flag}" ]; then
        return 0
    fi
    
    sleep ${timeout_hours}h
    
    # check running flag file exists
    if [ ! -f "${running_flag}" ]; then
        return 0
    fi
    
    # get running pid in ${running_flag}
    local running_pid=`cat ${running_flag}`
    if [ -z "${running_pid}" ] || [ ${running_pid} != ${watch_pid} ]; then
        return 0
    fi
    
    # check pid is still running and parrent pid is ${ppid}
    local running=`ps -ef | grep ${running_pid} | grep -v "grep" | grep ${ppid} | wc -l`
    if [ ${running} -eq 1 ]; then
        # alarm timeout
        alarm_timeout "${start_watching_time}"
    fi
    return 0
}

WORK_ROOT=$(cd "$(dirname "$0")"; pwd)
DEFAULT_CONF_FILE=module.conf
DEFAULT_TIMEOUT=5

module_name=""
conf_file=""
total_timeout=""
extra_args=""

while getopts :m:c:t:a:h opt; do
    case ${opt} in
    m) 
    module_name=$OPTARG
    ;;
    c) 
    conf_file=$OPTARG
    ;;
    t)
    total_timeout=$OPTARG
    check_timeout_val ${total_timeout}
    if [ $? -ne 0 ]; then
        echo "invalid timeout value: ${total_timeout}"
        usage `basename $0` 
        exit 1
    fi
    echo "set total timeout: ${total_timeout}!"
    ;;
    a)
    extra_args="$OPTARG"
    ;;
    h)
    usage `basename $0` 
    exit 0
    ;;
    :) 
    if [ ${OPTARG} == "p" ]; then
        p_flag=1
        start_job=""
    else
        echo "missing argument -"${OPTARG}
        usage `basename $0`
        exit 1
    fi
    ;;
    \?) echo "invalid argument -"${OPTARG}
    usage `basename $0`
    exit 1
    ;;
    esac
done

if [ -z ${module_name} ]; then
    echo "missing module name"
    usage `basename $0`
    exit 1
fi

if [ -z ${conf_file} ]; then
    conf_file=${DEFAULT_CONF_FILE}
    echo "conf file has not set yet! use [${conf_file}] as default"
fi

if [ -z ${total_timeout} ]; then
    total_timeout=${DEFAULT_TIMEOUT}
    echo "total timeout has not set yet! use ${total_timeout}h as default!"
fi

conf_full_path=${WORK_ROOT}/${module_name}/conf/${conf_file}
if [ ! -f "${conf_full_path}" ]; then
    echo "module conf [${conf_full_path}] not found!"
    exit 1
fi

# set env
export WORK_ROOT
export MODULE_CONF_FILE=${conf_full_path}
export MODULE_NAME=${module_name}

source ${conf_full_path}

if [ -z "${START_CMD}" ]; then
    echo "missing start command!"
    exit 1
fi

if [ ! -d "${LOCAL_DATA_PATH}" ] || [ ! -d ${LOCAL_LOG_PATH} ]; then
    echo "data_path [${LOCAL_DATA_PATH}] or log_path [${LOCAL_LOG_PATH}] are not dictionary!"
fi

if [ ! -s "${MODULE_WARNING_FILE}" ]; then
    echo "warninglevel conf: ${MODULE_WARNING_FILE} not found!"
    exit 1
fi
read_warning_level ${MODULE_WARNING_FILE}

mkdir -p ${LOCAL_DATA_PATH}
mkdir -p ${LOCAL_LOG_PATH}
time_stamp=`date  +"%Y%m%d%H%M%S"`
running_flag_file=${LOCAL_DATA_PATH}/flag.${module_name}.${time_stamp}.running

# run
${START_CMD} ${extra_args} &>${LOCAL_LOG_PATH}/${module_name}.log.${time_stamp} &
command_pid=$!

# set running flag
echo ${command_pid} &>${running_flag_file}
if [ $? -ne 0 ]; then
    msg="Failed to set running flag [${running_flag_file}]! Timeout checking will not take effect! Please check!"
    send_mail_msg -t 2 -s "$msg" -p "$msg"
    send_gsm_msg -t 2 -s "$msg"
fi

# run timeout watcher
current_pid=$$
watch_timeout ${total_timeout} ${command_pid} ${current_pid} ${running_flag_file} &
watcher_pid=$!

wait $command_pid &>/dev/null
ret=$?
rm -f ${running_flag_file}

# kill timeout watcher
watcher_exist=`ps -ef | grep ${watcher_pid} | grep -v "grep" | grep ${current_pid} | wc -l`
if [ ${watcher_exist} -eq 1 ]; then
    kill -9 ${watcher_pid} &>/dev/null
fi

if [ ${ret} -ne 0 ]; then
    alarm_failure ${ret} ${time_stamp}
    echo "Module ${module_name} failed with ret [${ret}]! Start at [${time_stamp}]!"
fi

exit $ret
