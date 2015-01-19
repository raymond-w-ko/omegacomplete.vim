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

  libs = ['boost_thread', 'boost_system', 'boost_chrono', 'boost_filesystem']
  path_prefix = '/usr/lib/lib' + libs[0] + '-mt'
  if os.path.exists(path_prefix + '.a') or os.path.exists(path_prefix + '.dll.a'):
    for i in xrange(len(libs)):
      libs[i] = libs[i] + '-mt'

  global_args = ['-std=c++11', '-march=native', '-fno-stack-protector', '-fstrict-aliasing', '-Ofast', '-flto']
  if sys.platform == 'cygwin':
    global_args.pop()

  compile_args = ['-Wall', '-Wno-char-subscripts']
  compile_args.extend(global_args)

  link_args = global_args

  module1 = Extension(
      'core',
      libraries = libs,
      extra_compile_args = compile_args,
      extra_link_args = link_args,
      sources = files)
else:
  global_args = ['-std=c++11', '-march=native', '-fno-stack-protector', '-Ofast', '-flto']

  compile_args = ['-Wall', '-Wno-char-subscripts', '-Wno-error=unused-command-line-argument']
  compile_args.extend(global_args)

  link_args = global_args

  module1 = Extension(
      'core',
      include_dirs = ['/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                      '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7/',
                      '/usr/local/include',
                      ],
      library_dirs = ['/usr/local/lib'],
      libraries = ['boost_thread-mt', 'boost_system-mt', 'boost_chrono-mt'],
      extra_compile_args = compile_args,
      extra_link_args = link_args,
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
