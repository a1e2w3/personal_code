#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os
import re
sys.path.insert(0, '')

def select_env_str(key, default_value=""):
    val = os.getenv(key)
    if val is None:
        val = default_value
        sys.stderr.write("select env [%s] failed! use [%s] as default!\n" % (key, val))
    else:
        sys.stderr.write("select env [%s] success! value: [%s]!\n" % (key, val))
    return val
    
def select_env_int(key, default_value=0):
    str_val = os.getenv(key)
    if str_val is None:
        sys.stderr.write("select env [%s] failed! use [%d] as default!\n" % (key, default_value))
        return default_value
    try:
        i_val = int(str_val)
        sys.stderr.write("select env [%s] success! value: [%d]!\n" % (key, i_val))
        return i_val
    except Exception, e:
        sys.stderr.write("parse env [%s][%s] failed! use [%d] as default!\n" % (key, str_val, default_value))
        return default_value

class Counter(object):
    def __init__(self):
        self._running_on_hadoop = (os.environ.get("mapred_task_id") is not None)
        self._local_counter_dict = {}
        
    def increase_counter(self, counter_group, counter_name, incr=1):
        if self._running_on_hadoop:
            sys.stderr.write("reporter:counter:%s,%s,%d\n" % (counter_group, counter_name, incr))
        else:
            counter_pair = (counter_group, counter_name)
            if counter_pair not in self._local_counter_dict:
                self._local_counter_dict[counter_pair] = incr
            else:
                self._local_counter_dict[counter_pair] += incr
    
    def print_counter(self):
        for (counter_pair, counter_value) in self._local_counter_dict.items():
            sys.stderr.write("reporter:counter:%s,%s,%d\n" % (counter_pair[0], counter_pair[1], counter_value))


class HadoopEnv(object):
    def __init__(self):
        self.map_tasks_num = select_env_int("mapred_map_tasks")
        self.reduce_tasks_num = select_env_int("mapred_reduce_tasks")
        self.mapred_job_id = select_env_str("mapred_job_id")
        self.mapred_task_id = select_env_str("mapred_task_id")
        self.map_input_file = select_env_str("map_input_file")
        self.mapred_work_output_dir = select_env_str("mapred_work_output_dir")
        self.mapred_working_dir = select_env_str("mapred_working_dir")
        self.fs_default_name = select_env_str("fs_default_name")
    
    def is_running_on_hadoop(self):
        return self.mapred_task_id != ""
    
