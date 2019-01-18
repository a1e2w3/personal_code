#!/bin/bash

send_mail_msg()
{
# 发送邮件的通用程序
# 用法：send_mail_msg -t "报警级别数" -s "简要邮件标题"  -p "附件或内容"
# 参数：
# 	-t: RD只需要提供warninglevel配置中的报警级别，取值为0-3。脚本会自动提取该级别的收件信息。
# 	-s: RD只需要提供最关键的，简洁的 [简要报警信息] 即可。其它 [类别][模块名][时间]  信息由脚本自动补全
# 	-p: 要么是一行简单的文字做正文，要么是一个附件文件。该文件可以无后缀，将使用mail发送，如果是.txt或.html文件，将用sendmail发送

# 返回值：
#  0: 正常发送 
# -1: 参数个数不对
# -2: 参数项不对（即没有-t,-s,-p等参数）
# -3: 邮件标题格式不符合规范
# -4: 有后缀的附件参数文件后缀不对（如果有后缀由必须为html或txt）
# -5: 发送有后缀附件邮件时错误
# -6: 发送无后缀附件邮件时错误
# -7: 发送内容邮件时错误
# -8: 配置文件warninglevel位置不对或不存在


local FUNC_NAME="send_mail_msg"

# check parameter 
if [ $# -ne 6 ];then
	echo "$FUNC_NAME parameter error!"
	echo "Usage: $FUNC_NAME -t "报警级别数" -s "简要邮件标题" -p '附件或内容' "
	return -1
fi

# 获取参数
OPTIND=1
while getopts "t:s:p:" opt;
do
	case "$opt" in
		t) opt_t=$OPTARG;
		;;
		s) opt_s=$OPTARG;
		;;
		p) opt_p=$OPTARG;
		;;
		*) echo "parameter item error!"; return -2;
		;;
	esac
done

# 检查warninglevel中，模块名是否为不空
if [ -z "${MODULE_NAME}" ];then
        echo "the conf item MODULE_NAME is black"
        return -8
fi

# 判断报警级别
if [ "$opt_t" -ge ${LEVEL_NUM} -o "$opt_t"  -ge 4 ];then
	echo "warnling level error!,must be 0-3"
	return -1
fi
case "${opt_t}" in
	"0") _WARNAME=${LEVEL_0_NAME};	_MAILIST=${LEVEL_0_MAILLIST};	_GSMLIST=${LEVEL_0_GSMLIST};
	;;
	"1") _WARNAME=${LEVEL_1_NAME};	_MAILIST=${LEVEL_1_MAILLIST};	_GSMLIST=${LEVEL_1_GSMLIST};
	;;
	"2") _WARNAME=${LEVEL_2_NAME};	_MAILIST=${LEVEL_2_MAILLIST};	_GSMLIST=${LEVEL_2_GSMLIST};
	;;
	"3") _WARNAME=${LEVEL_3_NAME};	_MAILIST=${LEVEL_3_MAILLIST};	_GSMLIST=${LEVEL_3_GSMLIST};
	;;
	"*") echo "parameter item error!"; return -2;
        ;;	
esac
local warnlevel=`echo $_WARNAME | tr [A-Z] [a-z]`
if [ X${warnlevel} = X"stat" ];then
	_DATE=`date +%Y%m%d`
else
	_DATE=`date +%H:%M:%S`
fi

_SUBJECT="[${_WARNAME}][${MODULE_NAME}][${opt_s}][${_DATE}]"

# 简单检查收件人是否规范，如果是个人，则调用函数处理
for tmpi_Qz4_tUw9Pg in  ${_MAILIST};do
        echo ${tmpi_Qz4_tUw9Pg} | grep -E 'spi|mon|-' &>/dev/null
        if [ $? -ne 0 ];then
                check_host_valid "${_MAILIST}"
    			break
	    fi
done

# # 将特定级别的报警邮件导入到EIP系统（默认导入Error,Fatal级邮件，不区别大小写）
# local maillevel=`echo $opt_s | awk -F']' '{print $1}' | sed 's/\[//g' | tr -s [A-Z] [a-z] `
# if [ $maillevel = "error" -o $maillevel = "fatal" ];then
#        /bin/mail_to_eip -s ${opt_s}
#        if [ $? -ne 0 ];then
#                echo "ERROR! 报警邮件导入EIP时错误，请检查"
#        fi
# fi


# 判断第三个参数，是内容还是文件，如果是文件，则进一步确认是否带后缀
if [  -z "$opt_p" ];then
	# 附件或内容参数为空
        mail -s "${_SUBJECT}" "${_MAILIST}" < /dev/null
	if [ $? -eq 0 ];then
                return 0
        else
                echo "发送内容邮件时错误，请检查"
                return -7
        fi
fi

if [ -f "$opt_p" ];then
        # 该参数为文件，判断后缀
	file_name=`echo "$opt_p" | awk -F'/' '{print $NF}'`
	echo $file_name | grep '\.' &>/dev/null
	if [ $? -eq 0 ];then
		# 带后缀
		sub_fix=`echo $file_name | awk -F'.' '{print $NF}' | tr [A-Z] [a-z]`
		if [ $sub_fix != "html" -a $sub_fix != "txt" ];then
			echo "附件参数文件后缀不为html或txt"
			return -4
		fi
		# 发送邮件	
	   cat "$opt_p" | formail -I "MIME-Version:1.0" -I "Content-type:text/html" -I "Subject:${_SUBJECT}" -I "To:${_MAILIST}" |/usr/sbin/sendmail -oi "${_MAILIST}"
		if [ $? -eq 0 ];then
			return 0
		else
			echo "发送带后缀附件邮件时错误，请检查"
			return -5
		fi
	else
		# 附件不带后缀，直接mail发送邮件
		cat "$opt_p" | mail -s "${_SUBJECT}" "${_MAILIST}"
		if [ $? -eq 0 ];then
                        return 0
                else
                        echo "发送无附件邮件时错误，请检查"
	                return -6
                fi
	fi

else
	# 该参数为内容，直接发送邮件	
        echo "$opt_p" | mail -s "${_SUBJECT}" "${_MAILIST}" 	
        if [ $? -eq 0 ];then
                return 0
        else
                echo "发送内容邮件时错误，请检查"
                return -7
        fi
fi

}

check_host_valid()
{
OP_USER=""
OP_MAIL_MASTER=
#OP_MAIL_MASTER=

local i;
local j=0;

if [  -z "$1" ];then
	echo "error: mail id is black"
	return -1
fi

for i in ${OP_USER} ;do
	id ${i} | grep '\-OP' &>/dev/null
	if [ $? -eq 0 ];then 
		let j=j+1
	fi
done
if [ ${j} -gt 3 ];then
	# 该机器OP有权限，则进一步去掉测试机
	echo "`hostname -s`" | grep -v '\-blmon[0-9]' | grep -v '\-test' &>/dev/null
	if [ $? -eq 0 ];then 
		# 发警告邮件给MAIL_MASTER，提醒spider线上机器的邮件报警可能存在异常收件人
		echo "Mail or gsm list include ( ${1} ) at `pwd`" | mail -s "[warn][send_msg.sh][`whoami`@`hostname -s`][${FUNC_NAME}收件人参数包括(${1}),可能不规范][`date +%H:%M:%S`]" "${OP_MAIL_MASTER}" 
		echo "waring! include personal postbox"
		return 0
	fi
else
	# OP无权限，说明是外组机器，或是组内非线上服务机器
	return 0
fi	
}

send_gsm_msg()
{
# 发送短信的通用程序
# 用法：send_gsm_msg  -t "报警级别数" -s "简要邮件标题"
# 参数：
#       -t: 报警级别数：RD只需要提供warninglevel配置中的报警级别，取值为0-3。脚本会自动提取该级别的收件信息。
#       -s: RD只需要提供最关键的，简洁的 [简要报警信息] 即可。其它 [类别][模块名][机器名][时间] 信息由脚本自动补全

# 返回值：
#  0: 正常发送 
# -1: 参数个数不对
# -2: 参数项不对（即没有-t,-s等参数）
# -3: 短信标题格式不符合规范
# -4: 发送短信内容错误
# -5: 配置文件warninglevel位置不对或不存在

local FUNC_NAME="send_gsm_msg"

# check parameter 
if [ $# -ne 4 ];then
        echo "$FUNC_NAME parameter error!"
        echo "Usage: $FUNC_NAME -t "报警级别数" -s "简要邮件标题""
        return -1
fi

# 获取参数
OPTIND=1
while getopts "t:s:" opt;
do
        case "$opt" in
                t) opt_t=$OPTARG;
                ;;
                s) opt_s=$OPTARG; 
                ;;
                *) echo "parameter item error!"; return -2;
                ;;
        esac
done

# 检查warninglevel中，模块名是否为不空
if [ -z "${MODULE_NAME}" ];then
        echo "the conf item MODULE_NAME is black"
        return -8
fi

# 判断报警级别
if [ "$opt_t" -ge ${LEVEL_NUM} -o "$opt_t"  -ge 4 ];then
        echo "warnling level error!,must be 0-3"
        return -1
fi

# 读取收件人信息
case "${opt_t}" in
        "0") _WARNAME=${LEVEL_0_NAME};  _MAILIST=${LEVEL_0_MAILLIST};   _GSMLIST=${LEVEL_0_GSMLIST};
        ;;
        "1") _WARNAME=${LEVEL_1_NAME};  _MAILIST=${LEVEL_1_MAILLIST};   _GSMLIST=${LEVEL_1_GSMLIST};
        ;;
        "2") _WARNAME=${LEVEL_2_NAME};  _MAILIST=${LEVEL_2_MAILLIST};   _GSMLIST=${LEVEL_2_GSMLIST};
        ;;
        "3") _WARNAME=${LEVEL_3_NAME};  _MAILIST=${LEVEL_3_MAILLIST};   _GSMLIST=${LEVEL_3_GSMLIST};
        ;;
        "*") echo "parameter item error!"; return -2;
        ;;
esac

_DATE=`date +%H:%M:%S`
_SUBJECT="[${_WARNAME}][${MODULE_NAME}][`hostname -s`][${opt_s}][${_DATE}]"

echo ${_GSMLIST}| grep 'g_psop_' &>/dev/null
if [ $? -ne 0 ];then
	check_host_valid "${_GSMLIST}"
fi

local gsmname=0
for tmpi_Qz4_tUw9Pg in ${_GSMLIST};do
	# Message format: phone_num@content	
	gsmsend  -s emp01.baidu.com:15001 -semp02.baidu.com:15001  ${tmpi_Qz4_tUw9Pg}@"\"${_SUBJECT}\""
	if [ $? -ne 0 ];then
               let gsmname=gsmname+1
        fi
done

if [ $gsmname -eq 0 ];then
	return 0           
else
	echo "发送短信内容存在异常,共${gsmname}次未成功，请检查"
    return -4
fi

}
