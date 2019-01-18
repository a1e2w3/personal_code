#!/bin/bash
set -x

function check_env()
{
    if [ -z $HADOOP_BIN ]; then
        echo "conf: HADOOP_BIN is needed!"
        return 1
    fi
    if [ -z $HADOOP_UGI ]; then
        echo "conf: HADOOP_UGI is needed!"
        return 1
    fi
    if [ -z $HDFS_WORK_PATH ]; then
        echo "conf: HDFS_WORK_PATH is needed!"
        return 1
    fi
    if [ -z $HDFS_TMP_PATH ]; then
        echo "conf: HDFS_TMP_PATH is needed!"
        return 1
    fi
    return 0
}

function check_distcp_conf() 
{
    if [ -z $DISTCP_JOB_NAME ]; then
        echo "conf: DISTCP_JOB_NAME is needed!"
        return 1
    fi
    if [ -z $DISTCP_SOURCE_FS ]; then
        echo "conf: DISTCP_SOURCE_FS is needed!"
        return 1
    fi
    if [ -z $DISTCP_SOURCE_UGI ]; then
        echo "conf: DISTCP_SOURCE_UGI is needed!"
        return 1
    fi
    if [ -z $DISTCP_INPUT_PATH ]; then
        echo "conf: DISTCP_INPUT_PATH is needed!"
        return 1
    fi
    if [ -z $DISTCP_OUTPUT_PATH ]; then
        echo "conf: DISTCP_OUTPUT_PATH is needed!"
        return 1
    fi
    if [ -z $DISTCP_TMP_PATH ]; then
        echo "conf: DISTCP_TMP_PATH is needed!"
        return 1
    fi

    # map task num
    if [ -z $MAP_TASKS ]; then
        MAP_TASKS=100
        echo "conf: MAP_TASKS not found! use ${MAP_TASKS} as default!"
    fi
    # map task capacity
    if [ -z $MAP_TASKS_CAPACITY ]; then
        MAP_TASKS_CAPACITY=100
        echo "conf: MAP_TASKS_CAPACITY not found! use ${MAP_TASKS_CAPACITY} as default!"
    fi
    # job priority
    if [ -z $JOB_PRIORITY ]; then
        JOB_PRIORITY="VERY_HIGH"
        echo "conf: JOB_PRIORITY not found! use [${JOB_PRIORITY}] as default!"
    fi

    return 0
}

function run_distcp()
{
    local timestamp=`date +%Y%m%d%H%M%S`
    local input_path=${DISTCP_SOURCE_FS}${DISTCP_INPUT_PATH}
    local output_path=${DISTCP_OUTPUT_PATH}
    local tmp_output_path=${DISTCP_TMP_PATH}/${DISTCP_JOB_NAME}_${timestamp}

    ${HADOOP_BIN} dfs -rmr ${tmp_output_path} &>/dev/null
    ${HADOOP_BIN} distcp -D mapred.job.name=${MAPRED_JOB_NAME} \
            -D mapred.map.tasks=${MAP_TASKS} \
            -D mapred.job.map.capacity=${MAP_TASKS_CAPACITY} \
            -D mapred.job.priority="${JOB_PRIORITY}" \
            -su "${DISTCP_SOURCE_UGI}" \
            -du "${HADOOP_UGI}" \
            ${input_path} \
            ${tmp_output_path} 

    distcp_ret=$?
    if [ ${distcp_ret} -ne 0 ]; then
        return ${distcp_ret}
    fi

    ${HADOOP_BIN} dfs -rmr ${output_path} &>/dev/null
    ${HADOOP_BIN} dfs -mv ${tmp_output_path} ${output_path}
    local mv_ret=$?
    if [ ${mv_ret} -ne 0 ]; then
        return ${mv_ret}
    fi
    return 0
}
    
if [ $# -lt 2 ]; then
    echo "usage: "$0" MODULE_CONF DISTCP_CONF [RUN_DATE]"
    exit 1
fi
module_conf_file=$1
distcp_conf=$2
source $module_conf_file
check_env 1>&2
if [ $? -ne 0 ]; then
    exit 1
fi

RUN_DATE=$3
# date flag
if [ -z "${DATE_FLAG}" ]; then
    DATE_FLAG=`date +"%Y%m%d"`
fi

if [ -n "${RUN_DATE}" ]; then
    DATE_FLAG=${RUN_DATE}
fi

source $distcp_conf
check_distcp_conf 1>&2
if [ $? -ne 0 ]; then
    exit 1
fi

run_distcp
exit $?

