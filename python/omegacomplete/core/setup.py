import sys
import os
from distutils.core import setup, Extension

files = [
  'Algorithm.cpp',
  'Buffer.cpp',
  'DllMain.cpp',
  'LookupTable.cpp',
  'Omegacomplete.cpp',
  'PythonMain.cpp',
  'Tags.cpp',
  'TagsCollection.cpp',
  'TestCases.cpp',
  'TrieNode.cpp',
  'WordCollection.cpp',
  'stdafx.cpp',
  ]

if sys.platform != 'darwin':
  libs = ['boost_thread', 'boost_system', 'boost_chrono']
  path_prefix = '/usr/lib/lib' + libs[0] + '-mt'
  if os.path.exists(path_prefix + '.a') or os.path.exists(path_prefix + '.dll.a'):
    for i in xrange(len(libs)):
      libs[i] = libs[i] + '-mt'
  # example module setup file for Cygwin and Linux
  module1 = Extension(
      'core',
      libraries = libs,
      extra_compile_args = ['-Wno-char-subscripts'],
      sources = files)
else:
  # example module setup file for Mac OS X 10.7
  module1 = Extension(
      'core',
      include_dirs = ['/Users/rko/boost',
                      '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                      '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7/'],
      library_dirs = ['/Users/rko/boost/stage/lib'],
      libraries = ['boost_thread', 'boost_system', 'boost_chrono'],
      extra_compile_args = ['-Wno-char-subscripts'],
      sources = files)

setup(name = 'core',
      version = '1.0',
      description = 'Omegacomplete Core',
      author = 'Raymond W. Ko',
      author_email = 'raymond.w.ko@gmail.com',
      url = 'https://github.com/raymond-w-ko/omegacomplete.vim',
      long_description = '''
This is the C++ portion of Omegacomplete. It is responsible for parsing buffers
and calculating completions.
''',
      ext_modules = [module1])
