import os

if not 'NSIS' in os.environ or not os.path.exists(os.environ['NSIS']):
  print ('DebugMode Frameserver needs NSIS to create the installer package. Please set the environment variable "NSIS" to the path of "makensis.exe".')
  Exit(1)

# Setup the environment for various versions of VC
if not 'VSCMD_ARG_TGT_ARCH' in os.environ or os.environ['VSCMD_ARG_TGT_ARCH'] != 'x64':
  print('We need to build x86 and x64 targets. So, please run scons from Visual Studio\'s "x64 Native Tools Command Prompt" terminal.')
  Exit(1)

isdbg = True if int(ARGUMENTS.get('dbg', 0)) == 1 else False
isdev = True if int(ARGUMENTS.get('dev', 0)) == 1 else False

program_files = os.environ['ProgramFiles']
if not program_files.endswith('\\'):
  program_files += '\\'

def SetupEnv(environ):
  # Add the most commonly used libs as the default ones to save work in each target.
  environ['LIBPATH'] = [
    '../fscommon',    # for the build targets one level below /src
    '../../fscommon'  # for the build targets two levels below /src
  ]
  environ['LIBS'] = [
    # Standard windows libraries
    'shlwapi.lib',
    'kernel32.lib',
    'user32.lib',
    'gdi32.lib',
    'winspool.lib',
    'comdlg32.lib',
    'advapi32.lib',
    'shell32.lib',
    'ole32.lib',
    'oleaut32.lib',
    'uuid.lib',
    'odbc32.lib',
    'odbccp32.lib',
    'winmm.lib',

    # Frameserver related libraries
    'common.lib',
  ]

  # If the environment variables INCLUDE and LIB were set, use them instead of the
  # default ones that SCons has created. This allows each developer to have their
  # own install dir for each SDK and point them to the build with the environment
  # variables.
  if 'INCLUDE' in os.environ:
    environ['ENV']['INCLUDE'] = environ['ENV']['INCLUDE'].rstrip(';') + ';' + os.environ['INCLUDE']
  if 'LIB' in os.environ:
    environ['ENV']['LIB'] = environ['ENV']['LIB'].rstrip(';') + ';' + os.environ['LIB']

  environ.AppendENVPath('PATH', os.path.join(program_files, 'Microsoft SDKs\\Windows\\v6.1\\Bin'))

  # Release configurations.
  if isdbg:
    out_dir = 'build/dbg'
    ccflags = ['/MDd', '/W3', '/Od', '/Fd', '/EHsc','/RTC1', '/Zi', '/errorReport:prompt']
    linkflags = ['/DEBUG' , '/MANIFEST', '/MANIFESTUAC:level=\'asInvoker\' uiAccess=\'false\'', '/SUBSYSTEM:WINDOWS', '/DYNAMICBASE' , '/NXCOMPAT', '/ERRORREPORT:PROMPT']
    cppdefines = ['_WINDOWS', 'UNICODE', '_UNICODE', '_DEBUG']
    rcflags = ['/l', '0x409', '/d', '_DEBUG']
  elif isdev:
    out_dir = 'build/dev'
    ccflags = ['/MD', '/W3', '/Od', '/Fd', '/Gy', '/EHsc', '/Zi', '/errorReport:prompt']
    linkflags = ['/DEBUG' , '/MANIFEST', '/MANIFESTUAC:level=\'asInvoker\' uiAccess=\'false\'', '/SUBSYSTEM:WINDOWS', '/DYNAMICBASE' , '/NXCOMPAT',  '/ERRORREPORT:PROMPT']
    cppdefines = ['_WINDOWS', 'UNICODE', '_UNICODE', 'NDEBUG', 'FSDEV']
  else:
    out_dir = 'build/opt'
    ccflags = ['/MD', '/W3', '/O2', '/Oi', '/GL', '/FD', '/Gy', '/EHsc', '/Zi', '/errorReport:prompt']
    linkflags = ['/DEBUG' , '/MANIFEST', '/MANIFESTUAC:level=\'asInvoker\' uiAccess=\'false\'', '/SUBSYSTEM:WINDOWS', '/DYNAMICBASE' , '/NXCOMPAT', '/OPT:REF', '/OPT:ICF', '/LTCG', '/ERRORREPORT:PROMPT']
    cppdefines = ['_WINDOWS', 'UNICODE', '_UNICODE', 'NDEBUG']
  ccpdbflags = ['${(PDB and "/Zi") or ""}']
  rcflags = ['/l', '0x409', '/d', '_DEBUG']

  environ['CCFLAGS'] = ccflags
  environ['LINKFLAGS'] = linkflags
  environ['CPPDEFINES'] = cppdefines
  environ['CCPDBFLAGS'] = ccpdbflags
  environ['CXXFLAGS'] = ['$(', '/TP', '$)'] #as in SCONS v2.x
  environ['RCFLAGS'] += rcflags
  environ['dbg'] = 1 if isdbg else 0
  environ['NSISFLAGS'] = ['/V2']
  environ.Tool("nsis", toolpath=["scons_tools"])
  environ.VariantDir(out_dir, 'src', duplicate=0)
  environ['NSIS'] = os.environ['NSIS']
  if environ['TARGET_ARCH'] == 'x86_64':
    environ['AS'] = "ml64"  # work around a bug with scons using 'ml' for assembler files rather than ml64

'''
Latest build was done with MS Visual Studio 2017 and Windows 10 SDK, version 1903
'''

env = Environment(TARGET_ARCH='x86_64')
env32 = Environment(TARGET_ARCH='x86')
SetupEnv(env)
SetupEnv(env32)
Export('env')
Export('env32')

# Build!
if isdbg:
  env.SConscript('build/dbg/SConscript')
elif isdev:
  env.SConscript('build/dev/SConscript')
else:
  env.SConscript('build/opt/SConscript')
