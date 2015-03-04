# Copyright (c) 2015 Cloudwatt
# All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License.
#
# @author: Numan Siddique, eNovance

from cfgm_common import importutils
from gen_py.sandesh.ttypes import SandeshLevel
import logging


class SandeshBaseLogger(object):
    """Sandesh Base Logger"""

    _logger = None

    _SANDESH_LEVEL_TO_LOGGER_LEVEL = {
        SandeshLevel.SYS_EMERG : logging.CRITICAL,
        SandeshLevel.SYS_ALERT : logging.CRITICAL,
        SandeshLevel.SYS_CRIT : logging.CRITICAL,
        SandeshLevel.SYS_ERR : logging.ERROR,
        SandeshLevel.SYS_WARN : logging.WARNING,
        SandeshLevel.SYS_NOTICE : logging.WARNING,
        SandeshLevel.SYS_INFO : logging.INFO,
        SandeshLevel.SYS_DEBUG : logging.DEBUG
    }

    def __init__(self, generator, logger_config_file=None):
        pass

    @staticmethod
    def create_logger(generator, logger_class=None,
                      logger_config_file=None):
        l_class = importutils.import_class(logger_class)
        return l_class(generator,
                       logger_config_file=logger_config_file)

    @staticmethod
    def get_py_logger_level(sandesh_level):
        return SandeshBaseLogger._SANDESH_LEVEL_TO_LOGGER_LEVEL[sandesh_level]
    #end get_py_logger_level

    def logger(self):
        return self._logger

    def set_logging_params(self, *args, **kwargs):
        pass
