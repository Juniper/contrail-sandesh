#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

from setuptools import setup

setup(
    name='sandesh',
    version='0.1dev',
    packages = [
                'pysandesh',
                'pysandesh.protocol',
                'pysandesh.transport',
                'pysandesh.gen_py',
                'pysandesh.gen_py.sandesh',
                'pysandesh.gen_py.sandesh_alarm',
                'pysandesh.gen_py.sandesh_ctrl',
                'pysandesh.gen_py.sandesh_uve',
                'pysandesh.gen_py.sandesh_dynamic_uve',
                'pysandesh.gen_py.sandesh_uve.io',
                'pysandesh.gen_py.sandesh_trace',
                'pysandesh.gen_py.process_info',
               ],
    package_data={
                  'pysandesh':['webs/*.xsl','webs/js/*.js','webs/css/*.css','webs/css/images/*.png'],
                  'pysandesh.gen_py.sandesh_uve':['*.html', '*.css', '*.xml'],
                  'pysandesh.gen_py.sandesh_alarm':['*.html', '*.css', '*.xml'],
                  'pysandesh.gen_py.sandesh_uve.io':['*.html', '*.css', '*.xml'],
                  'pysandesh.gen_py.sandesh_ctrl' : ['*.html', '*.css', '*.xml'],
                  'pysandesh.gen_py.sandesh_trace':['*.html', '*.css', '*.xml'],
                  'pysandesh.gen_py.process_info':['*.html', '*.css', '*.xml'],
                 },
    long_description="Sandesh python Implementation",
    install_requires=[
                      'gevent',
                     ]
)
