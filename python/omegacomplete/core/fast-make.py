import sys
import os
import platform
from distutils.core import setup, Extension

files = []
for root, dirs, fs in os.walk("."):
  for file in fs:
    if file.endswith(".cpp"):
      files.append(file)
  break

if sys.platform != 'darwin':

  libs = ['boost_thread', 'boost_system', 'boost_chrono', 'boost_filesystem']
  path_prefix = '/usr/lib/lib' + libs[0] + '-mt'
  if os.path.exists(path_prefix + '.a') or os.path.exists(path_prefix + '.dll.a'):
    for i in xrange(len(libs)):
      libs[i] = libs[i] + '-mt'

  global_args = ['-std=c++11', '-mtune=native', '-march=native', '-fno-stack-protector', '-fstrict-aliasing', '-O0']
  if platform.system().startswith("CYGWIN"):
    global_args.append("-D_GNU_SOURCE")
  #global_args = ['-std=c++11', '-fstrict-aliasing', '-O0', '-ggdb']

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
  global_args = ['-std=c++11', '-mtune=native', '-march=native', '-fno-stack-protector', '-O0']

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
