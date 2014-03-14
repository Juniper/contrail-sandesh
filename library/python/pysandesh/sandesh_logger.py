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
        self._logging_category = ''
        self._logging_file = self._DEFAULT_LOG_FILE
        self._logger = logging.getLogger(self._generator)
        self._logging_level = SandeshLevel.SYS_INFO
        self._logger.setLevel(self._SANDESH_LEVEL_TO_LOGGER_LEVEL[self._logging_level])
        self._logging_handler = logging.StreamHandler()
        log_format = logging.Formatter('%(asctime)s [%(name)s]: %(message)s',
                                       datefmt='%m/%d/%Y %I:%M:%S %p')
        self._logging_handler.setFormatter(log_format)
        self._logger.addHandler(self._logging_handler)
    #end __init__

    def logger(self):
        return self._logger
    #end logger

    def set_logging_params(self, enable_local_log=False, category='', level=SandeshLevel.SYS_INFO,
                           file=_DEFAULT_LOG_FILE):
        self.set_local_logging(enable_local_log)
        self.set_logging_category(category)
        self.set_logging_level(level)
        self.set_logging_file(file)
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
    
    def set_logging_file(self, file):
        if self._logging_file != file:
            self._logger.info('SANDESH: Logging: FILE: [%s] -> [%s]',
                              self._logging_file, file)
            self._logger.removeHandler(self._logging_handler)
            if file == self._DEFAULT_LOG_FILE:
                self._logging_handler = logging.StreamHandler()
            else:
                self._logging_handler = logging.handlers.RotatingFileHandler(filename=file,
                                                                             maxBytes=5000000,
                                                                             backupCount=10)
            log_format = logging.Formatter('%(asctime)s [%(name)s]: %(message)s',
                                           datefmt='%m/%d/%Y %I:%M:%S %p')
            self._logging_handler.setFormatter(log_format)
            self._logger.addHandler(self._logging_handler) 
            self._logging_file = file   
    #end set_logging_file        
            
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
    
#end class SandeshLogger
