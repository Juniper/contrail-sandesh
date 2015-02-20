#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# Sandesh Logger
#

import logging
import logging.handlers
from gen_py.sandesh.ttypes import SandeshLevel

class SandeshLogger(object):

    """Sandesh Logger Implementation."""

    _logger = None
    _DEFAULT_LOG_FILE = '<stdout>'
    _DEFAULT_SYSLOG_FACILITY = 'LOG_LOCAL0'

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

    def __init__(self, generator):
        assert generator, 'SandeshLogger init requires generator name'

        self._generator = generator
        self._enable_local_log = False
        self._enable_syslog = False
        self._logging_category = ''
        self._logging_file = self._DEFAULT_LOG_FILE
        self._logging_syslog_facility = self._DEFAULT_SYSLOG_FACILITY
        self._logger = logging.getLogger(self._generator)
        self._logging_level = SandeshLevel.SYS_INFO
        self._logger.setLevel(SandeshLogger.get_py_logger_level(self._logging_level))
        if not len(self._logger.handlers):
            # add the handler only once
            self._logging_file_handler = logging.StreamHandler()
            log_format = logging.Formatter('%(asctime)s [%(name)s]: %(message)s',
                                           datefmt='%m/%d/%Y %I:%M:%S %p')
            self._logging_file_handler.setFormatter(log_format)
            self._logger.addHandler(self._logging_file_handler)
        else:
            self._logging_file_handler = self._logger.handlers[0]
    #end __init__

    @staticmethod
    def get_py_logger_level(sandesh_level):
        return SandeshLogger._SANDESH_LEVEL_TO_LOGGER_LEVEL[sandesh_level]
    #end get_py_logger_level

    def logger(self):
        return self._logger
    #end logger

    def set_logging_params(self, enable_local_log=False, category='', level=SandeshLevel.SYS_INFO,
                           file=_DEFAULT_LOG_FILE, enable_syslog=False, syslog_facility='LOG_LOCAL0',
                           max_file_bytes=None, file_backup_count=None, log_formatter=None):
        self.set_local_logging(enable_local_log)
        self.set_logging_category(category)
        self.set_logging_level(level)
        self.set_logging_file(file, max_file_bytes=max_file_bytes,
                              file_backup_count=file_backup_count,
                              log_formatter=log_formatter)
        self.set_logging_syslog(enable_syslog, syslog_facility)
    #end set_logging_params

    def set_local_logging(self, enable_local_log):
        if self._enable_local_log != enable_local_log:
            self._logger.info('SANDESH: Logging: %s -> %s', self._enable_local_log, enable_local_log)
            self._enable_local_log = enable_local_log
    #end set_local_logging

    def set_logging_level(self, level):
        if isinstance(level, str):
            if level in SandeshLevel._NAMES_TO_VALUES:
                level = SandeshLevel._NAMES_TO_VALUES[level]
            else:
                level = SandeshLevel.SYS_INFO
        # get logging level corresponding to sandesh level
        try:
            logger_level = self._SANDESH_LEVEL_TO_LOGGER_LEVEL[level]
        except KeyError:
            logger_level = logging.INFO
            level = SandeshLevel.SYS_INFO

        if self._logging_level != level:
            self._logger.info('SANDESH: Logging: LEVEL: [%s] -> [%s]',
                              SandeshLevel._VALUES_TO_NAMES[self._logging_level],
                              SandeshLevel._VALUES_TO_NAMES[level])
            self._logging_level = level
            self._logger.setLevel(logger_level)
    #end set_logging_level

    def set_logging_category(self, category):
        if self._logging_category != category:
            self._logger.info('SANDESH: Logging: CATEGORY: %s -> %s', self._logging_category, category)
            self._logging_category = category
    #end set_logging_category

    def set_logging_file(self, file, max_file_bytes=None, file_backup_count=None,
                         log_formatter=None):
        if self._logging_file != file:
            self._logger.info('SANDESH: Logging: FILE: [%s] -> [%s]',
                              self._logging_file, file)
            self._logger.removeHandler(self._logging_file_handler)
            if file == self._DEFAULT_LOG_FILE:
                self._logging_file_handler = logging.StreamHandler()
            else:
                if not max_file_bytes:
                    max_file_bytes = 5000000

                if not file_backup_count:
                    file_backup_count = 10

                self._logging_file_handler = logging.handlers.RotatingFileHandler(
                    filename=file, maxBytes=max_file_bytes,
                    backupCount=file_backup_count)
            if log_formatter:
                fmt = log_formatter.get('fmt',
                                        '%(asctime)s [%(name)s]: %(message)s')
                datefmt = log_formatter.get('datefmt', '%m/%d/%Y %I:%M:%S %p')
            else:
                fmt = '%(asctime)s [%(name)s]: %(message)s'
                datefmt = '%m/%d/%Y %I:%M:%S %p'

            log_format = logging.Formatter(fmt=fmt, datefmt=datefmt)

            self._logging_file_handler.setFormatter(log_format)
            self._logger.addHandler(self._logging_file_handler)
            self._logging_file = file
    #end set_logging_file

    def set_logging_syslog(self, enable_syslog, syslog_facility):
        if self._enable_syslog == enable_syslog and \
           self._logging_syslog_facility == syslog_facility:
            return

        if self._logging_syslog_facility != syslog_facility:
            self._logger.info('SANDESH: Logging: SYSLOG: [%s] -> [%s]',
                              self._logging_syslog_facility, syslog_facility)

        if self._enable_syslog:
            self._logger.removeHandler(self._logging_syslog_handler)

        if enable_syslog:
            self._logging_syslog_handler = logging.handlers.SysLogHandler(
                address="/dev/log",
                facility=getattr(logging.handlers.SysLogHandler, syslog_facility,
                                 logging.handlers.SysLogHandler.LOG_LOCAL0)
            )
            self._logger.addHandler(self._logging_syslog_handler)

        self._enable_syslog = enable_syslog
        self._logging_syslog_facility = syslog_facility
    #end set_logging_syslog

    def is_local_logging_enabled(self):
        return self._enable_local_log
    #end is_local_logging_enabled

    def logging_level(self):
        return self._logging_level
    #end logging_level

    def logging_category(self):
        return self._logging_category
    #end logging_category

    def logging_file(self):
        return self._logging_file
    #end logging_file

    def is_syslog_logging_enabled(self):
        return self._enable_syslog_log
    #end is_syslog_logging_enabled

    def logging_syslog_facility(self):
        return self._logging_syslog_facility
    #end logging_syslog_facility

#end class SandeshLogger
