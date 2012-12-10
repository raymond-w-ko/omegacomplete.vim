from distutils.core import setup, Extension

# example module setup file for Cygwin and Linux
module1 = Extension('core',
                    libraries = ['boost_thread-mt', 'boost_system-mt'],
                    sources = ['Algorithm.cpp', 'Buffer.cpp', 'DllMain.cpp',
                        'GlobalWordSet.cpp', 'LookupTable.cpp',
                        'OmegaComplete.cpp', 'PythonMain.cpp', 'stdafx.cpp',
                        'Tags.cpp', 'TagsSet.cpp', 'TrieNode.cpp',
                        'ustring.cpp'])

# example module setup file for Mac OS X 10.7
#module1 = Extension('core',
                    #include_dirs = ['/Users/rko/boost',
                                    #'/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                                    #'/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7/'],
                    #library_dirs = ['/Users/rko/boost/stage/lib'],
                    #libraries = ['boost_thread', 'boost_system'],
                    #sources = ['Algorithm.cpp', 'Buffer.cpp', 'DllMain.cpp',
                        #'GlobalWordSet.cpp', 'LookupTable.cpp',
                        #'OmegaComplete.cpp', 'PythonMain.cpp', 'stdafx.cpp',
                        #'Tags.cpp', 'TagsSet.cpp', 'TrieNode.cpp',
                        #'ustring.cpp'])

setup(name = 'core',
      version = '1.0',
      description = 'OmegaComplete Core',
      author = 'Raymond W. Ko',
      author_email = 'raymond.w.ko@gmail.com',
      url = 'https://github.com/raymond-w-ko/omegacomplete.vim',
      long_description = '''
This is the C++ portion of OmegaComplete. It is responsible for parsing buffers
and calculating completions.
''',
      ext_modules = [module1])
