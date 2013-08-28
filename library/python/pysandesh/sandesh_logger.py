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

    def __init__(self, module):
        assert module, 'SandeshLogger init requires module name'

        self._module = module
        self._enable_local_log = False
        self._logging_category = ''
        self._logging_level = SandeshLevel.SYS_INFO
        self._logging_file = self._DEFAULT_LOG_FILE
        self._logger = logging.getLogger(self._module)
        # For now, set the logging level=DEBUG
        self._logger.setLevel(logging.DEBUG)
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
        if self._logging_level != level:
            self._logger.info('SANDESH: Logging: LEVEL: [%s] -> [%s]', 
                              SandeshLevel._VALUES_TO_NAMES[self._logging_level],
                              SandeshLevel._VALUES_TO_NAMES[level])
            self._logging_level = level
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
