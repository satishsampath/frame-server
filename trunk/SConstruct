import os

def SetupEnv(environ, vcver):
  # Add the most commonly used libs as the default ones to save work in each target.
  environ['LIBPATH'] = [
    '../fscommon',    # for the build targets one level below /src
    '../../fscommon'  # for the build targets two levels below /src
  ]
  environ['LIBS'] = [
    # Standard windows libraries
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
    ccflags = ['/MTd', '/W3', '/Od', '/FD', '/GZ']
    rcflags = ['/l', '0x409', '/d', '_DEBUG']
    linkflags = ['/machine:I386']
    cppdefines = ['WIN32', '_WINDOWS', '_MBCS', '_DEBUG']
  else:
    out_dir = 'build/opt'
    ccflags = ['/MD', '/W3', '/O1', '/FD']
    rcflags = ['/l', '0x409', '/d', '_DEBUG']
    linkflags = ['/machine:I386', '/filealign:512', '/map', '/opt:ref', '/opt:icf']
    cppdefines = ['WIN32', '_WINDOWS', '_MBCS', 'NDEBUG']
  if vcver == 6:
    ccflags += ['/GX']
  else:
    ccflags += ['/EHsc']
  environ['CCFLAGS'] += ccflags
  environ['RCFLAGS'] += rcflags
  environ['LINKFLAGS'] = linkflags
  environ['CPPDEFINES'] = cppdefines
  environ['dbg'] = int(ARGUMENTS.get('dbg', 0))
  environ['NSISFLAGS'] = ['/V2']
  environ.Tool("nsis", toolpath=["scons_tools"])
  environ.VariantDir(out_dir, 'src', duplicate=0)

# Choose VC6 no matter what other Visual studio versions are installed
env = Environment(MSVS_VERSION = '6.0')
env_vc8 = Environment(MSVS_VERSION = '8.0')

SetupEnv(env, 6)
SetupEnv(env_vc8, 8)
Export('env')
Export('env_vc8')

# Build!
if int(ARGUMENTS.get('dbg', 0)):
  env.SConscript('build/dbg/SConscript')
else:
  env.SConscript('build/opt/SConscript')
