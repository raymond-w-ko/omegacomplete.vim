from distutils.core import setup, Extension

module1 = Extension('core',
                    define_macros = [('MAJOR_VERSION', '1'),
                                     ('MINOR_VERSION', '0')],
                    libraries = ['boost_thread-mt', 'boost_system-mt'],
                    sources = ['Algorithm.cpp', 'Buffer.cpp', 'DllMain.cpp', 'GlobalWordSet.cpp', 'LookupTable.cpp', 'OmegaComplete.cpp', 'Participant.cpp', 'PythonMain.cpp', 'Room.cpp', 'stdafx.cpp', 'Tags.cpp', 'TagsSet.cpp', 'Teleprompter.cpp', 'TrieNode.cpp', 'ustring.cpp'])

setup (name = 'core',
       version = '1.0',
       description = 'OmegaComplete Core',
       author = 'Raymond W. Ko',
       author_email = 'raymond.w.ko@gmail.com',
       url = 'https://github.com/raymond-w-ko/omegacomplete.vim',
       long_description = '''
This is the C++ portion of OmegaComplete
''',
       ext_modules = [module1])
