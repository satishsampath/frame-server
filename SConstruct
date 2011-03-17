import os

program_files = os.environ['ProgramFiles']
if not program_files.endswith('\\'):
  program_files += '\\'

def SetupEnv(environ, vcver):
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
  if os.environ.has_key('INCLUDE'):
    environ['ENV']['INCLUDE'] = environ['ENV']['INCLUDE'].rstrip(';') + ';' + os.environ['INCLUDE']
  if os.environ.has_key('LIB'):
    environ['ENV']['LIB'] = environ['ENV']['LIB'].rstrip(';') + ';' + os.environ['LIB']

  if vcver == 'vc9_x64':
    environ['ENV']['LIB'] = (os.path.join(program_files, 'Microsoft Visual Studio 9.0\\VC\\LIB\\amd64') + ';' +
                             os.path.join(program_files, 'Microsoft SDKs\\Windows\\v6.1\\lib\\x64'))
    environ.PrependENVPath('PATH', os.path.join(program_files, 'Microsoft Visual Studio 9.0\\VC\\bin\\x86_amd64'))

  environ.AppendENVPath('PATH', os.path.join(program_files, 'Microsoft SDKs\\Windows\\v6.1\\Bin'))

  # Release configurations.
  if int(ARGUMENTS.get('dbg', 0)):
    out_dir = 'build/dbg'
    ccflags9_x64 = ['/MDd', '/W3', '/Od', '/Fd', '/Gm', '/EHsc','/RTC1', '/Zi', '/errorReport:prompt']
    ccpdbflags9_x64 = ['${(PDB and "/Zi") or ""}']
    linkflags9_x64 = ['/machine:X64', '/DEBUG' , '/MANIFEST', '/MANIFESTUAC:level=\'asInvoker\' uiAccess=\'false\'', '/SUBSYSTEM:WINDOWS', '/DYNAMICBASE' , '/NXCOMPAT', '/ERRORREPORT:PROMPT']
    cppdefines9_x64 = ['WIN64', 'UNICODE', '_UNICODE', '_DEBUG']
    ccflags9 = ['/MDd', '/W3', '/Od', '/Fd', '/Gm', '/EHsc','/RTC1', '/ZI', '/errorReport:prompt']
    ccpdbflags9 = ['${(PDB and "/ZI") or ""}']
    linkflags9 = ['/machine:X86', '/DEBUG' , '/MANIFEST', '/MANIFESTUAC:level=\'asInvoker\' uiAccess=\'false\'', '/SUBSYSTEM:WINDOWS', '/DYNAMICBASE' , '/NXCOMPAT', '/ERRORREPORT:PROMPT']
    cppdefines9 = ['WIN32', '_WINDOWS', 'UNICODE', '_UNICODE', '_DEBUG']
    ccflags = ['/MTd', '/W3', '/Od', '/FD', '/GZ']
    rcflags = ['/l', '0x409', '/d', '_DEBUG']
    linkflags = ['/machine:I386']
    cppdefines = ['WIN32', '_WINDOWS', '_MBCS', '_DEBUG']
  else:
    out_dir = 'build/opt'
    ccflags9_x64 = ['/MD', '/W3', '/O2', '/Oi', '/GL', '/FD', '/Gy', '/EHsc', '/Zi', '/errorReport:prompt']
    ccpdbflags9_x64 = ['${(PDB and "/Zi") or ""}']
    linkflags9_x64 = ['/machine:X64', '/DEBUG' , '/MANIFEST', '/MANIFESTUAC:level=\'asInvoker\' uiAccess=\'false\'', '/SUBSYSTEM:WINDOWS', '/DYNAMICBASE' , '/NXCOMPAT', '/OPT:REF', '/OPT:ICF', '/LTCG', '/ERRORREPORT:PROMPT']
    cppdefines9_x64 = ['WIN64', 'UNICODE', '_UNICODE', 'NDEBUG']
    ccflags9 = ['/MD', '/W3', '/O2', '/Oi', '/GL', '/FD', '/EHsc', '/Zi', '/errorReport:prompt']
    ccpdbflags9 = ['${(PDB and "/Zi") or ""}']
    linkflags9 = ['/machine:X86', '/DEBUG' , '/MANIFEST', '/MANIFESTUAC:level=\'asInvoker\' uiAccess=\'false\'', '/SUBSYSTEM:WINDOWS', '/DYNAMICBASE' , '/NXCOMPAT', '/OPT:REF', '/OPT:ICF', '/LTCG', '/ERRORREPORT:PROMPT']
    cppdefines9 = ['WIN32', '_WINDOWS', 'UNICODE', '_UNICODE', 'NDEBUG']
    ccflags = ['/MD', '/W3', '/O1', '/FD']
    rcflags = ['/l', '0x409', '/d', '_DEBUG']
    linkflags = ['/machine:I386', '/filealign:512', '/map', '/opt:ref', '/opt:icf']
    cppdefines = ['WIN32', '_WINDOWS', '_MBCS', 'NDEBUG']
  if vcver == 'vc6':
    ccflags += ['/GX']
  else:
    ccflags += ['/EHsc']

  if vcver == 'vc9_x64':
    environ['CCFLAGS'] = ccflags9_x64
    environ['LINKFLAGS'] = linkflags9_x64
    environ['CPPDEFINES'] = cppdefines9_x64
    environ['CCPDBFLAGS'] = ccpdbflags9_x64
  elif vcver == 'vc9':
    environ['CCFLAGS'] = ccflags9
    environ['LINKFLAGS'] = linkflags9
    environ['CPPDEFINES'] = cppdefines9
    environ['CCPDBFLAGS'] = ccpdbflags9
  else:
    environ['CCFLAGS'] += ccflags
    environ['LINKFLAGS'] = linkflags
    environ['CPPDEFINES'] = cppdefines

  environ['CXXFLAGS'] = ['$(', '/TP', '$)'] #as in SCONS v2.x
  environ['RCFLAGS'] += rcflags
  environ['dbg'] = int(ARGUMENTS.get('dbg', 0))
  environ['NSISFLAGS'] = ['/V2']
  environ.Tool("nsis", toolpath=["scons_tools"])
  environ.VariantDir(out_dir, 'src', duplicate=0)

# Check that the relevant build tools and libraries are installed
path = os.path.join(program_files + 'Microsoft SDKs\\Windows\\v6.1\\Bin')
if not os.path.exists(path):
  print ('Unable to find Windows SDK 6.1 at "%s". Please download and install '
         'Windows Software Development Kit (SDK) for Windows Server 2008 and .NET Framework 3.5' % path)
  Exit(1)
paths = [
  os.path.join(program_files, 'Microsoft SDKs\\Windows\\v6.1\\lib\\x64'),
  os.path.join(program_files, 'Microsoft Visual Studio 9.0\\VC\\LIB\\amd64'),
  os.path.join(program_files, 'Microsoft Visual Studio 9.0\\VC\\bin\\x86_amd64')
]
for path in paths:
  if not os.path.exists(path):
    print ('Unable to find x64 components of Windows SDK 6.1 at "%s". Please download and install '
           'Windows Software Development Kit (SDK) for Windows Server 2008 and .NET Framework 3.5 '
           'along with the x64 components.' % path)
    Exit(1)

# Setup the environment for various versions of VC
env = Environment(MSVS_VERSION = '6.0')
env_vc9 = Environment(MSVS_VERSION = '9.0')
env_vc9_x64 = Environment(MSVS_VERSION = '9.0')

SetupEnv(env, 'vc6')
SetupEnv(env_vc9, 'vc9')
SetupEnv(env_vc9_x64, 'vc9_x64')
Export('env')
Export('env_vc9')
Export('env_vc9_x64')

# Build!
if int(ARGUMENTS.get('dbg', 0)):
  env.SConscript('build/dbg/SConscript')
else:
  env.SConscript('build/opt/SConscript')
