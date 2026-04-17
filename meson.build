project('controld', 'c',
  version : '1.0.0',
  default_options : ['warning_level=3', 'c_std=gnu11'])

# Font's
src_daemon = ['src/controld.c']
src_client = ['src/controlctl.c']

executable('controld', src_daemon, install : true)
executable('controlctl', src_client, install : true)