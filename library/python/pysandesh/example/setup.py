#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

from setuptools import setup

setup(
    name='pysandeshexample',
    version='0.1.dev0',
    packages=[
              'pysandesh_example',
              'pysandesh_example.gen_py',
              'pysandesh_example.gen_py.generator_msg',
              'pysandesh_example.gen_py.vm',
              'pysandesh_example.gen_py.vn'
             ],
    package_data={'pysandesh_example.gen_py.vm':['*.html', '*.css', '*.xml'],
                  'pysandesh_example.gen_py.vn':['*.html', '*.css', '*.xml'],
                  'pysandesh_example.gen_py.generator_msg':['*.html', '*.css', '*.xml'],
                 },
    zip_safe=False,
    long_description="Pysandesh Example Package",
    install_requires=[
                      'gevent',
                     ]
)
