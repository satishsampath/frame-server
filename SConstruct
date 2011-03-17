import os

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
    environ['ENV']['INCLUDE'] = (environ['ENV']['INCLUDE'].rstrip(';') + ';' +
                                 os.environ['INCLUDE'])
  if os.environ.has_key('LIB'):
    environ['ENV']['LIB'] = (environ['ENV']['LIB'].rstrip(';') + ';' +
                             os.environ['LIB'])

  # Release configurations.
  if int(ARGUMENTS.get('dbg', 0)):
    out_dir = 'build/dbg'
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
    ccflags9 = ['/MD', '/W3', '/O2', '/Oi', '/GL', '/FD', '/EHsc', '/Zi', '/errorReport:prompt']
    ccpdbflags9 = ['${(PDB and "/Zi") or ""}']
    linkflags9 = ['/machine:X86', '/DEBUG' , '/MANIFEST', '/MANIFESTUAC:level=\'asInvoker\' uiAccess=\'false\'', '/SUBSYSTEM:WINDOWS', '/DYNAMICBASE' , '/NXCOMPAT', '/OPT:REF', '/OPT:ICF', '/LTCG', '/ERRORREPORT:PROMPT']
    cppdefines9 = ['WIN32', '_WINDOWS', 'UNICODE', '_UNICODE', 'NDEBUG']
    ccflags = ['/MD', '/W3', '/O1', '/FD']
    rcflags = ['/l', '0x409', '/d', '_DEBUG']
    linkflags = ['/machine:I386', '/filealign:512', '/map', '/opt:ref', '/opt:icf']
    cppdefines = ['WIN32', '_WINDOWS', '_MBCS', 'NDEBUG']
  if vcver == 6:
    ccflags += ['/GX']
  else:
    ccflags += ['/EHsc']

  if vcver == 9:
    environ['CCFLAGS'] = ccflags9
    environ['LINKFLAGS'] = linkflags9
    environ['CPPDEFINES'] = cppdefines9
    environ['CCPDBFLAGS'] = ccpdbflags9
    mtpaths=[
      'c:\\Program Files\\Microsoft SDKs\\Windows\\v7.1\\Bin',
      'c:\\Program Files (x86)\\Microsoft SDKs\\Windows\\v7.1\\Bin',
      'c:\\Program Files\\Microsoft SDKs\\Windows\\v7.0a\\Bin',
      'c:\\Program Files (x86)\\Microsoft SDKs\\Windows\\v7.0a\\Bin'
    ]
    found = False
    for p in mtpaths:
      mt = os.path.join(p, 'mt.exe')
      if os.path.exists(mt):
        found = True
        environ.AppendENVPath('PATH', p)
        break
    if not found:
      print "Error: Unable to find where mt.exe and rc.exe are installed. Please check if you have the latest Windows SDK installed."
      Exit(1)
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

# Choose VC6 no matter what other Visual studio versions are installed
env = Environment(MSVS_VERSION = '6.0')
env_vc8 = Environment(MSVS_VERSION = '8.0')
env_vc9 = Environment(MSVS_VERSION = '9.0')


SetupEnv(env, 6)
SetupEnv(env_vc8, 8)
SetupEnv(env_vc9, 9)
Export('env')
Export('env_vc8')
Export('env_vc9')

# Build!
if int(ARGUMENTS.get('dbg', 0)):
  env.SConscript('build/dbg/SConscript')
else:
  env.SConscript('build/opt/SConscript')
